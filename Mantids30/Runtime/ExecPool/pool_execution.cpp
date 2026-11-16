#include "pool.h"

#ifndef _WIN32
#include <utility>

using namespace Mantids30::Runtime;
using Clock = std::chrono::steady_clock;

ExecPool::Error ExecPool::execute(const std::shared_ptr<State> &state)
{
    // Claim the request once and retain the lock until the condition-variable wait.
    std::unique_lock<std::mutex> lock(state->mutex);
    if (state->used)
    {
        return Error::ALREADY_EXECUTED;
    }
    state->used = true;
    {
        // Admission and shutdown share this lock to avoid stranded queued requests.
        std::scoped_lock submission(m_submission);
        Error rejected = Error::SUCCESS;
        if (!m_running)
        {
            rejected = Error::NOT_RUNNING;
        }
        else
        {
            // Use one monotonic deadline for queue residence, writing and reading.
            // Queue insertion is nonblocking so overload becomes an explicit rejection.
            state->submitted = Clock::now();
            state->deadline = state->submitted + m_config.requestTimeout;
            auto item = std::make_unique<std::shared_ptr<State>>(state);
            if (!m_queue.push(item.get(), 0))
            {
                rejected = Error::QUEUE_FULL;
            }
            else
            {
                // Queue now owns the wrapper; the shared state also remains owned by
                // this caller until completion, even if a worker removes the wrapper.
                item.release();
            }
        }
        std::scoped_lock stats(m_statisticsMutex);
        if (rejected != Error::SUCCESS)
        {
            ++m_statistics.rejected;
            state->error = rejected;
            state->done = true;
            return rejected;
        }
        ++m_statistics.submitted;
    }
    // The predicate handles both spurious wakes and completion before waiting.
    // A worker can still win the completion race after wait_until reports timeout.
    if (!state->completed.wait_until(lock, state->deadline, [&] { return state->done; }))
    {
        // finish() acquires the state mutex and arbitrates races with worker completion.
        lock.unlock();
        finish(state, Error::TIMEOUT);
        lock.lock();
    }
    return state->error;
}

void ExecPool::finish(const std::shared_ptr<State> &state, Error error, boost::json::value output)
{
    std::scoped_lock lock(state->mutex);
    // Timeout, shutdown and worker completion may race; only the first result counts.
    if (state->done)
    {
        return;
    }
    // Publish payload, status and done under one lock so accessors never observe
    // a partially published result. Count accepted work only once in this path.
    state->output = std::move(output);
    state->error = error;
    state->done = true;
    {
        std::scoped_lock stats(m_statisticsMutex);
        ++m_statistics.completed;
        if (error == Error::SUCCESS)
        {
            ++m_statistics.succeeded;
        }
        else
        {
            ++m_statistics.failed;
        }
        if (error == Error::TIMEOUT)
        {
            ++m_statistics.timedOut;
        }
        m_statistics.totalLatencyMicroseconds +=
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - state->submitted).count();
    }
    // Notify all waiters sharing this state; their predicate observes done under mutex.
    state->completed.notify_all();
}
#endif
