#pragma once

#ifndef _WIN32
#include <Mantids30/Core/Helpers/appexec.h>
#include <Mantids30/Runtime/Logs/applog.h>
#include <Mantids30/Core/Threads/safe_queue.h>
#include <Mantids30/DataFormat/JSON/json.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace Mantids30::Runtime {

/**
 * @brief Manages persistent POSIX children using a JSON-lines request/response protocol.
 *
 * Each worker serves one request at a time. Configuration is immutable after
 * construction; start()/stop() are serialized and Request::exec() supports
 * concurrent callers. Each response must contain exactly one JSON line.
 *
 * @note Failed requests are never replayed. Statistics accumulate across restarts.
 * @note The pool must outlive all concurrent calls into its API.
 */
class ExecPool
{
public:
    /**
     * @brief Admission, execution and protocol outcomes for a request.
     */
    enum class Error : std::uint8_t
    {
        SUCCESS, ///< Response validated before the deadline.
        NOT_RUNNING, ///< Pool was stopped when admission was attempted.
        QUEUE_FULL, ///< Queue capacity was exhausted at admission.
        TIMEOUT, ///< The absolute request deadline expired.
        STOPPED, ///< Pool shutdown cancelled the request.
        SPAWN_FAILED, ///< Worker had no child after a spawn attempt.
        PROCESS_EXITED, ///< Child exited or closed a required stream.
        IO_ERROR, ///< A stream or polling operation failed.
        INVALID_JSON, ///< Response line could not be parsed as JSON.
        DESYNCHRONIZED, ///< Response framing or correlation ID did not match.
        LINE_TOO_LONG, ///< Response exceeded maxLineBytes before its newline.
        INVALID_INPUT, ///< Correlation IDs require an object input.
        INPUT_TOO_LONG, ///< Serialized input exceeded maxLineBytes.
        ALREADY_EXECUTED, ///< This shared request state has already been submitted.
        INTERNAL_ERROR ///< An unexpected worker exception interrupted execution.
    };

    /**
     * @brief Recovery policy after a worker process or exchange failure.
     */
    enum class FailureBehavior : std::uint8_t
    {
        RESTART_WORKER, ///< Replace the failed child after restartDelay; do not replay its request.
        STOP_POOL       ///< Stop admission and cancel queued work across all workers.
    };

    /**
     * @brief Immutable process settings and execution limits.
     */
    struct Config
    {
        /**
         * @brief Optional application logger for pool diagnostics.
         *
         * Null disables logging. The pool retains shared ownership until destruction.
         * Configure the logger before starting the pool; worker threads log concurrently.
         * Request and response payloads are never included in diagnostics.
         */
        std::shared_ptr<Logs::AppLog> appLog;

        /**
         * @brief Executable path and arguments; executed directly without a shell or PATH lookup.
         */
        Core::Helpers::AppExec::ExecutionArgs command;

        /**
         * @brief Inherited environment overrides in NAME=VALUE form.
         */
        std::vector<std::string> environment;

        /**
         * @brief Number of persistent children; must be positive.
         */
        size_t workers{1};

        /**
         * @brief Maximum waiting requests, excluding active exchanges; must be positive.
         */
        size_t maxQueuedRequests{1024};

        /**
         * @brief Includes queue, write and read time.
         */
        std::chrono::milliseconds requestTimeout{30000};

        /**
         * @brief Excludes the terminating newline.
         */
        size_t maxLineBytes{size_t{1024} * 1024};

        /**
         * @brief Backoff between process restarts.
         */
        std::chrono::milliseconds restartDelay{100};

        /**
         * @brief Policy applied after child or exchange failures.
         */
        FailureBehavior failureBehavior{FailureBehavior::RESTART_WORKER};

        /**
         * @brief Empty disables IDs; otherwise input/output must be objects.
         */
        std::string idField{"_exec_id"};

        /**
         * @brief Empty inherits stderr. Otherwise prefix + "-N" + suffix.
         */
        std::string stderrFilePrefix;

        /**
         * @brief Files are opened in append mode, including after restart.
         */
        std::string stderrFileSuffix{".log"};

    };

    /**
     * @brief Cumulative accepted-work counters and current occupancy gauges.
     */
    struct Statistics
    {
        uint64_t submitted{0}; ///< Requests accepted into the queue.
        uint64_t completed{0}; ///< Accepted requests with a published final result.
        uint64_t succeeded{0}; ///< Completed requests with SUCCESS.
        uint64_t failed{0}; ///< Completed requests with any non-success status.
        uint64_t timedOut{0}; ///< Completed requests with TIMEOUT; included in failed.
        uint64_t rejected{0}; ///< Admission failures; excludes ALREADY_EXECUTED calls.
        uint64_t processesStarted{0}; ///< Successful child creations, including replacements.
        uint64_t processesRestarted{0}; ///< Successful replacement creations after initial startup.
        uint64_t processFailures{0}; ///< Detected idle-child or attempted-exchange failures requiring recovery.
        uint64_t totalLatencyMicroseconds{0}; ///< Completed accepted requests, including queue time.
        size_t queued{0}; ///< Queue size sampled by statistics(); may include requests already timed out.
        size_t active{0}; ///< Requests currently handled by workers, possibly already completed by caller timeout.
    };

    /**
     * @brief Represents a synchronous, one-shot JSON request.
     *
     * Completion is published through shared state protected by a mutex and
     * condition variable. Copies share execution state; output and status can
     * be inspected concurrently. The pool must outlive all exec() calls.
     */
    class Request
    {
    public:
        /**
         * @brief Constructs a request with its own shared execution state.
         * @param input JSON payload copied or moved into the request.
         * @note Copies of Request share the same state and one-shot execution limit.
         */
        explicit Request(boost::json::value input);

        /**
         * @brief Submits the request and blocks until completion or timeout.
         * @param pool Pool used to execute this request; must outlive this call.
         * @return Completion status, admission failure, or ALREADY_EXECUTED on reuse.
         *
         * The timeout includes queueing and process I/O. A rejected request is also
         * considered used. Concurrent calls sharing this request admit at most one execution.
         * @note Failed or timed-out requests are never replayed automatically.
         */
        Error exec(ExecPool &pool);

        /**
         * @brief Returns a synchronized copy of the response payload.
         * @return Response JSON after success; initially null and null on failure.
         * @note This accessor does not wait for completion.
         */
        [[nodiscard]] boost::json::value output() const;

        /**
         * @brief Reads the last published request status.
         * @return NOT_RUNNING initially, then the rejection or completion status.
         * @note This accessor does not wait; a pending request retains its initial status.
         */
        [[nodiscard]] Error error() const;

    private:
        /**
         * @brief Shared lifetime and completion state retained independently of a request handle.
         */
        struct State
        {
            /**
             * @brief Initializes the state shared by request handles, callers and workers.
             * @param value Payload retained unchanged for the lifetime of the state.
             * @note mutex protects completion, reuse detection, output and error.
             */
            explicit State(boost::json::value value) : input(std::move(value)) {}
            std::mutex mutex;
            std::condition_variable completed;
            boost::json::value input, output;
            Error error{Error::NOT_RUNNING};
            bool used{false}, done{false};
            std::chrono::steady_clock::time_point submitted, deadline;
        };
        std::shared_ptr<State> m_state;
        friend class ExecPool;
    };

    /**
     * @brief Constructs a stopped pool with immutable configuration.
     * @param config Command, queue limits, deadlines and worker recovery policy.
     * @throws std::invalid_argument If workers, queue capacity or line limit is zero,
     *         timeout is nonpositive, restart delay is negative or executable is empty.
     * @note Executable validation and process creation occur in start().
     */
    explicit ExecPool(Config config);

    /**
     * @brief Stops the pool and releases owned worker processes and threads.
     * @pre No caller may access the pool during or after destruction.
     */
    ~ExecPool();

    /**
     * @brief Disables copying of process, thread and synchronization ownership.
     */
    ExecPool(const ExecPool &) = delete;

    /**
     * @brief Disables assignment of process, thread and synchronization ownership.
     */
    ExecPool &operator=(const ExecPool &) = delete;

    /**
     * @brief Creates child processes and starts their consumer threads.
     * @param error Optional destination for a failure description; untouched on success.
     * @return true if already running or startup succeeds; false after startup cleanup.
     *
     * A previous stopped generation is joined before new workers are created.
     * Partial startup is rolled back before returning false.
     * @note Serialized with stop(); accumulated statistics are retained.
     */
    bool start(std::string *error = nullptr);

    /**
     * @brief Stops admission, cancels queued work and joins consumer threads.
     *
     * Active exchanges observe shutdown during polling; workers terminate and reap
     * their direct children before exiting. Descendant processes are not managed.
     * @note Safe to call repeatedly; serialized with start().
     */
    void stop();

    /**
     * @brief Queries whether the pool currently permits execution.
     * @return Current value of the atomic running flag.
     * @note This snapshot does not reserve capacity or guarantee later admission.
     */
    bool isRunning() const noexcept;

    /**
     * @brief Reads cumulative counters and current queue occupancy.
     * @return Counters retained across restarts, plus queued and active work counts.
     * @note Queue occupancy is sampled separately and can change during the snapshot.
     */
    Statistics statistics() const;

private:
    using State = Request::State;
    using App = Core::Helpers::AppSpawn;

    /**
     * @brief Child and consumer ownership for one worker generation.
     */
    struct Worker
    {
        std::unique_ptr<App> app;
        std::thread thread;
        std::chrono::steady_clock::time_point nextSpawn;
    };
    const Config m_config;
    // Lifecycle protects the worker vector; submission gates admission and draining.
    // Statistics has its own lock so counters can be sampled during execution.
    mutable std::mutex m_lifecycle, m_submission, m_statisticsMutex;
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_nextId{1};
    // Queue owns heap wrappers; each wrapper shares request state with its caller.
    mutable Core::Threads::Safe::Queue<std::shared_ptr<State>> m_queue;
    std::vector<std::unique_ptr<Worker>> m_workers;
    Statistics m_statistics;

    /**
     * @brief Admits a shared state once and waits for its first completion.
     * @param state Request state kept alive throughout queueing and execution.
     * @return Admission error, ALREADY_EXECUTED, or the published completion status.
     * @note Admission is serialized with shutdown; waiting releases the state mutex.
     */
    Error execute(const std::shared_ptr<State> &state);

    /**
     * @brief Publishes completion exactly once and notifies waiting callers.
     * @param state Accepted request whose completion is being attempted.
     * @param error Final execution status.
     * @param output Response payload, normally supplied only for SUCCESS.
     * @note Late completions are ignored, including worker results after caller timeout.
     * @pre The caller does not hold state->mutex.
     */
    void finish(const std::shared_ptr<State> &state, Error error, boost::json::value output = {});

    /**
     * @brief Creates a child process with nonblocking request and response streams.
     * @param index Zero-based worker index used for its stderr filename.
     * @param restart Whether successful creation also increments the restart counter.
     * @return Owned process on success, or null on executable, spawn or pipe setup failure.
     * @note Arguments and environment configuration can throw; callers handle exceptions.
     */
    std::unique_ptr<App> spawn(size_t index, bool restart);

    /**
     * @brief Consumes queued requests and manages one child process.
     * @param worker Process, consumer thread and restart deadline for this worker.
     * @param index Zero-based worker index used when replacing the child.
     * @note Unexpected exceptions complete current work as INTERNAL_ERROR and stop admission.
     *       The owned child is terminated before this function returns.
     */
    void consume(Worker &worker, size_t index) noexcept;

    /**
     * @brief Sends one JSON line and validates one correlated response.
     * @param app Child process with nonblocking stdin and stdout.
     * @param state Input and absolute deadline of the current request.
     * @param output Destination for parsed response JSON; use only on SUCCESS.
     * @return SUCCESS or an input, transport, protocol, timeout or shutdown error.
     * @note The deadline covers partial writes and reads; polling also observes shutdown.
     */
    Error exchange(App &app, const std::shared_ptr<State> &state, boost::json::value &output);

    /**
     * @brief Stops admission and completes queued requests as STOPPED.
     * @note May run on a worker thread; does not join threads or acquire m_lifecycle.
     */
    void failPool();

    /**
     * @brief Emits a diagnostic without allowing logging failures to affect execution.
     * @param error Outcome to describe; SUCCESS and normal STOPPED cancellation are ignored.
     * @param context Operation that failed, using a fixed internal description.
     * @param detail Optional diagnostic text, never interpreted as a format string.
     * @param worker One-based worker number, or zero for a pool/request event.
     * @param systemError Saved errno value, or zero when no system error applies.
     * @note Uses WARNING for admission, input and timeout errors; ERROR for other failures.
     */
    void logProblem(Error error, const char *context, const char *detail = "", size_t worker = 0, int systemError = 0) const noexcept;

    /**
     * @brief Removes queued wrappers and publishes STOPPED for unfinished requests.
     * @note Already completed states retain their first result.
     * @pre Callers exclude admission through m_submission or a stopped lifecycle.
     */
    void drain();
    friend class Request;
};

} // namespace Mantids30::Runtime
#endif
