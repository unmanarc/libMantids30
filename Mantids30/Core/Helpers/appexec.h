#pragma once

#ifndef _WIN32
#include <cstdio>
#include <poll.h>
#include <set>
#include <spawn.h>
#endif

#include <string>
#include <vector>

// Define STDOUT_FILENO/STDERR_FILENO:
#include <cstdint>
#include <unistd.h>

namespace Mantids30::Helpers {

/**
 * @brief Provides blocking command execution with output capture.
 */
class AppExec
{
public:
    /**
     * @brief Execution infrastructure status.
     *
     * @note SUCCESS indicates that execution completed successfully at the
     *       infrastructure level. Inspect ExecutionResult::exitCode to determine
     *       whether the command itself succeeded.
     */
    enum class Status : uint8_t
    {
        SUCCESS = 0,                           ///< Execution completed.
        PIPE_CREATION_FAILED = 1,              ///< Failed to create an output pipe.
        HANDLE_INFORMATION_SETTING_FAILED = 2, ///< Failed to configure handle information.
        WAIT_FOR_PROCESS_FAILED = 3,           ///< Failed to wait for the process.
        UNKNOWN_ERROR = 4,                     ///< An unclassified execution error occurred.
        PROCESS_CREATION_FAILED = 5            ///< Failed to create the process.
    };

    /**
     * @brief Captured output and result of a blocking execution.
     */
    struct ExecutionResult
    {
        /**
         * @brief Output captured during execution.
         */
        std::string output;

        /**
         * @brief Command exit code, normally zero on success.
         *
         * @note Interpret this value only when @ref status indicates SUCCESS.
         */
        int exitCode;

        /**
         * @brief Status of the execution infrastructure.
         */
        AppExec::Status status;
    };

    /**
     * @brief Executable and arguments for a command.
     */
    struct ExecutionArgs
    {
        /**
         * @brief Executable name or path.
         */
        std::string arg0;

        /**
         * @brief Command arguments, excluding @ref arg0.
         */
        std::vector<std::string> args;
    };

    /**
     * @brief Constructs an execution helper.
     */
    AppExec() = default;

    /**
     * @brief Executes a command and blocks until execution completes.
     * @param cmd Executable and command arguments.
     * @return Captured output, command exit code, and execution status.
     */
    static ExecutionResult blexec(const ExecutionArgs &cmd);
};

#ifndef _WIN32

/**
 * @brief Spawns a POSIX child process with optional output redirection.
 *
 * Configure the executable, arguments, environment overrides, and file
 * redirections before calling spawnProcess(). Captured streams can be consumed
 * through pollResponse() and read().
 *
 * Each instance supports one successful spawn per reset(). Failed spawn attempts
 * preserve the configuration so execution can be retried.
 *
 * @warning Drain both captured streams before calling waitUntilProcessEnds().
 *          Otherwise, the child can block while writing to a full pipe.
 * @warning Destruction and reset() close captured streams and wait for the
 *          child. They can block indefinitely if the child keeps running.
 * @note Instances require external synchronization for concurrent access.
 */
class AppSpawn
{
public:
    /**
     * @brief Constructs an unconfigured process launcher.
     */
    AppSpawn();

    /**
     * @brief Closes captured streams and waits for the child.
     *
     * @note Closing captured streams can cause SIGPIPE or EPIPE in the child.
     */
    ~AppSpawn();

    AppSpawn(const AppSpawn &) = delete;
    AppSpawn &operator=(const AppSpawn &) = delete;
    AppSpawn(AppSpawn &&) = delete;
    AppSpawn &operator=(AppSpawn &&) = delete;

    /**
     * @brief Sets the executable path.
     * @param path Path to an executable regular file.
     * @return true if validation succeeds; otherwise false with errno set.
     *
     * @note A failed validation clears the previous executable path.
     *       A call rejected because a process was already spawned preserves it.
     * @note Validation does not guarantee that subsequent execution will succeed.
     * @pre No successful spawn has occurred since construction or the last reset().
     */
    bool setExec(const std::string &path);

    /**
     * @brief Schedules a file to be opened as a child file descriptor.
     * @param outFile Path to the output file.
     * @param fd Child descriptor to redirect; defaults to standard output.
     * @return true if the action is recorded; otherwise false with errno set.
     *
     * The file is opened during spawning with O_WRONLY | O_CREAT | O_TRUNC.
     * Newly created files request mode 0644, subject to the process umask.
     * File actions are applied in registration order, before pipe redirections
     * and the optional stderr-to-stdout redirection.
     *
     * @note Successful registration does not guarantee that opening will succeed.
     * @pre No successful spawn has occurred since construction or the last reset().
     */
    bool addOpenFDToFile(const std::string &outFile, int fd = 1);

    /**
     * @brief Schedules standard error to duplicate the final standard output.
     * @return true if configured; otherwise false with errno set.
     *
     * Applied after file and pipe redirections. Takes precedence over the
     * pipeStderr argument of spawnProcess().
     *
     * @pre No successful spawn has occurred since construction or the last reset().
     */
    bool redirectStdErrToStdOut();

    /**
     * @brief Appends a command argument.
     * @param arg Argument passed verbatim, without shell interpretation.
     * @throws std::logic_error If a process was already successfully spawned.
     * @throws std::invalid_argument If arg contains an embedded NUL character.
     *
     * @note The executable supplies argv[0]; do not add it as an argument.
     */
    void addArgument(const std::string &arg);

    /**
     * @brief Adds or replaces a child environment variable.
     * @param env Assignment in NAME=VALUE form.
     * @throws std::logic_error If a process was already successfully spawned.
     * @throws std::invalid_argument If the assignment is malformed or contains NUL.
     *
     * The child inherits the current environment with these overrides applied.
     * For repeated names, the last registered assignment takes precedence.
     */
    void addEnvironment(const std::string &env);

    /**
     * @brief Spawns the configured executable.
     * @param pipeStdout Capture standard output through a pipe.
     * @param pipeStderr Capture standard error through a separate pipe.
     * @return true if spawning succeeds; otherwise false with errno set.
     *
     * On success, arguments and environment overrides are cleared. On failure,
     * configuration is preserved and any newly created pipes are closed.
     *
     * If redirectStdErrToStdOut() was requested, pipeStderr is ignored and
     * standard error follows the final standard output destination.
     *
     * @note Executes the configured path directly without PATH lookup or a shell.
     * @note Failure can still leave file-system effects from child file actions,
     *       such as a created or truncated output file.
     * @pre An executable has been configured with setExec().
     * @pre No successful spawn has occurred since construction or the last reset().
     */
    bool spawnProcess(bool pipeStdout = false, bool pipeStderr = false);

    /**
     * @brief Blocks until the child is reaped.
     * @throws std::system_error If waitpid() fails with an error other than ECHILD.
     *
     * Calls interrupted by signals are retried. Has no effect if no child was
     * spawned or the child has already been reaped.
     *
     * @pre Captured output has been drained, or another consumer is draining it.
     * @note Does not return the child's exit code.
     */
    void waitUntilProcessEnds();

    /**
     * @brief Waits for captured streams to become readable or reach EOF.
     * @return A set containing STDOUT_FILENO and/or STDERR_FILENO.
     *         Returns an empty set when no captured streams remain open.
     * @throws std::system_error If polling fails or reports a descriptor error.
     *
     * Call read() for each returned descriptor. A returned descriptor can have
     * pending data, EOF, or both in sequence; continue reading through subsequent
     * poll cycles until a positive-size read returns zero.
     *
     * @note Stream EOF does not imply that the child has terminated.
     */
    std::set<int> pollResponse();

    /**
     * @brief Reads from a captured child stream.
     * @param fd STDOUT_FILENO or STDERR_FILENO.
     * @param buf Destination buffer.
     * @param count Maximum number of bytes to read.
     * @return Bytes read, zero for EOF or a zero-size request, or -1 with errno set.
     *
     * Interrupted reads are retried. A positive-size read returning zero closes
     * the captured stream and removes it from subsequent polling.
     *
     * @note Returns -1 with errno set to EBADF for an unavailable or invalid stream.
     * @note May block when called without a preceding readiness notification.
     */
    ssize_t read(int fd, void *buf, size_t count);

    /**
     * @brief Reports whether the child has been reaped or waitpid() reported ECHILD.
     * @return true if the recorded child is no longer waitable; otherwise false.
     *
     * @note Reads cached state; it does not query the operating system.
     *       Initially false and restored to false by reset().
     */
    [[nodiscard]] bool hasExited() const;

    /**
     * @brief Clears all configuration and restores the initial state.
     * @throws std::system_error If waiting for the previous child fails.
     *
     * Closes captured streams, waits for the previous child, and clears the
     * executable, arguments, environment overrides, and redirection settings.
     *
     * @note Closing captured streams can cause SIGPIPE or EPIPE in the child.
     * @note If waiting fails, configuration is retained but pipes are already closed.
     */
    void reset();

private:
    /**
     * @brief Deferred child file-opening action.
     */
    struct OpenFD
    {
        std::string path; ///< File path to open during spawning.
        int fd;           ///< Target child descriptor.
    };

    std::string m_execPath;                 ///< Configured executable path.
    std::vector<std::string> m_arguments;   ///< Arguments excluding argv[0].
    std::vector<std::string> m_environment; ///< Registered environment overrides.
    std::vector<OpenFD> m_openFiles;        ///< Ordered file-opening actions.

    pid_t m_childPid{0};         ///< Child PID, or zero when unset/reaped.
    int m_piStdOut[2]{-1, -1};   ///< Stdout pipe: read and write ends.
    int m_piStdErr[2]{-1, -1};   ///< Stderr pipe: read and write ends.
    std::vector<pollfd> m_plist; ///< Open captured read descriptors.

    bool m_pipedStdout{false};       ///< Stdout capture remains open.
    bool m_pipedStderr{false};       ///< Stderr capture remains open.
    bool m_dupStderrToStdout{false}; ///< Merge stderr into final stdout.
    bool m_spawned{false};           ///< A spawn succeeded since the last reset.
    bool m_childExited{false};       ///< Child reaped or ECHILD observed.

    /**
     * @brief Closes all owned pipe ends and clears polling state.
     * @note Idempotent; preserves errno.
     */
    void closePipes() noexcept;

    /**
     * @brief Closes a captured stream and removes it from polling.
     * @param fd STDOUT_FILENO or STDERR_FILENO; other values are ignored.
     * @note Idempotent.
     */
    void closeStream(int fd) noexcept;

    /**
     * @brief Waits for and reaps the child, retrying interrupted waits.
     * @return true if no wait is needed, the child is reaped, or ECHILD is reported.
     *         Otherwise false with errno set.
     */
    bool reapChild() noexcept;
};
#endif

} // namespace Mantids30::Helpers