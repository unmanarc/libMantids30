#include "pool.h"

#ifndef _WIN32
#include <utility>

using namespace Mantids30::Runtime;
using Clock = std::chrono::steady_clock;

void ExecPool::consume(Worker &worker, size_t index) noexcept
{
    std::shared_ptr<State> current;
    bool active = false;
    try
    {
        while (m_running)
        {
            if (worker.app)
            {
                // Idle output violates request/response framing; discard that process.
                pollfd idle{worker.app->getStreamFD(STDOUT_FILENO), POLLIN, 0};
                const bool exited = !worker.app->isRunning();
                const int ready = ::poll(&idle, 1, 0);
                if (exited || (ready > 0 && idle.revents != 0))
                {
                    logProblem(exited ? Error::PROCESS_EXITED : Error::DESYNCHRONIZED,
                               "Idle worker failed", "", index + 1);
                    worker.app->terminate();
                    worker.app.reset();
                    worker.nextSpawn = Clock::now() + m_config.restartDelay;
                    {
                        std::scoped_lock stats(m_statisticsMutex);
                        ++m_statistics.processFailures;
                    }
                    if (m_config.failureBehavior == FailureBehavior::STOP_POOL)
                    {
                        failPool();
                        break;
                    }
                }
            }
            if (!worker.app)
            {
                // Bound restart frequency while checking shutdown regularly.
                if (Clock::now() < worker.nextSpawn)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                    continue;
                }
                worker.app = spawn(index, true);
                worker.nextSpawn = Clock::now() + m_config.restartDelay;
                if (!worker.app && m_config.failureBehavior == FailureBehavior::STOP_POOL)
                {
                    failPool();
                    break;
                }
            }
            // A short queue wait bounds shutdown latency when no requests arrive.
            // Taking ownership immediately also reclaims the wrapper on exceptions.
            std::unique_ptr<std::shared_ptr<State>> item(m_queue.pop(25));
            if (!item)
            {
                continue;
            }
            current = *item;
            {
                std::scoped_lock lock(current->mutex);
                // A queued request may already have timed out in its caller.
                if (current->done)
                {
                    current.reset();
                    continue;
                }
            }
            {
                std::scoped_lock stats(m_statisticsMutex);
                ++m_statistics.active;
                active = true;
            }
            boost::json::value output;
            Error error;
            // Avoid touching the process for shutdown or an expired queued request.
            // Neither condition means that the child itself has failed.
            bool attempted = false;
            if (!m_running)
            {
                error = Error::STOPPED;
            }
            else if (Clock::now() >= current->deadline)
            {
                error = Error::TIMEOUT;
            }
            else
            {
                attempted = true;
                error = worker.app ? exchange(*worker.app, current, output) : Error::SPAWN_FAILED;
            }
            // Local input rejection leaves the child usable; exchange failures may desynchronize it.
            const bool processFailure = attempted && error != Error::SUCCESS && error != Error::INVALID_INPUT &&
                error != Error::INPUT_TOO_LONG && error != Error::STOPPED;
            // Discard potentially unread bytes by replacing the whole process.
            // The failed request is completed below and is never re-enqueued.
            if (processFailure)
            {
                logProblem(error, "Worker exchange failed; applying failure policy", "", index + 1);
                worker.nextSpawn = Clock::now() + m_config.restartDelay;
                if (worker.app)
                {
                    worker.app->terminate();
                    worker.app.reset();
                }
                {
                    std::scoped_lock stats(m_statisticsMutex);
                    ++m_statistics.processFailures;
                }
                if (m_config.failureBehavior == FailureBehavior::STOP_POOL)
                {
                    failPool();
                }
            }
            // Caller timeout may already have published a result; finish() preserves it.
            // Only successful exchanges expose their parsed output to the caller.
            finish(current, error, error == Error::SUCCESS ? std::move(output) : boost::json::value{});
            current.reset();
            {
                std::scoped_lock stats(m_statisticsMutex);
                --m_statistics.active;
                active = false;
            }
        }
    }
    // Never let an exception escape the thread; complete work and stop further admission.
    catch (...)
    {
        logProblem(Error::INTERNAL_ERROR, "Worker exception; stopping pool", "", index + 1);
        if (current)
        {
            finish(current, Error::INTERNAL_ERROR);
        }
        // Balance active even if an exception bypassed the normal completion path.
        if (active)
        {
            std::scoped_lock stats(m_statisticsMutex);
            --m_statistics.active;
        }
        failPool();
    }
    // Each consumer owns cleanup of its direct child on normal shutdown as well
    // as policy-driven failure. stop() joins before releasing the Worker object.
    if (worker.app)
    {
        worker.app->terminate();
        worker.app.reset();
    }
}
#endif
