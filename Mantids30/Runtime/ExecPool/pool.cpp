#include "pool.h"

#ifndef _WIN32
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <utility>

using namespace Mantids30::Runtime;

ExecPool::ExecPool(Config config) : m_config(std::move(config))
{
    // Reject unusable limits before allocating workers or launching any child.
    if (m_config.workers == 0 || m_config.maxQueuedRequests == 0 || m_config.maxLineBytes == 0 ||
        m_config.requestTimeout.count() <= 0 || m_config.restartDelay.count() < 0 || m_config.command.arg0.empty())
    {
        throw std::invalid_argument("ExecPool: invalid configuration");
    }
    m_queue.setMaxItems(m_config.maxQueuedRequests);
}

ExecPool::~ExecPool()
{
    stop();
}

bool ExecPool::start(std::string *error)
{
    // Serialize lifecycle changes so worker ownership remains stable.
    std::scoped_lock lifecycle(m_lifecycle);
    if (m_running)
    {
        return true;
    }
    // A worker may have requested STOP_POOL. Join the previous generation first.
    for (auto &worker : m_workers)
    {
        if (worker->thread.joinable())
        {
            worker->thread.join();
        }
    }
    m_workers.clear();
    drain();
    try
    {
        // Create every child first. A failure here leaves no consumer threads to join.
        m_workers.reserve(m_config.workers);
        for (size_t index = 0; index < m_config.workers; ++index)
        {
            auto worker = std::make_unique<Worker>();
            worker->app = spawn(index, false);
            if (!worker->app)
            {
                throw std::system_error(errno, std::generic_category(), "ExecPool: process creation failed");
            }
            m_workers.push_back(std::move(worker));
        }
        // Keep submissions blocked until every consumer has been created.
        std::scoped_lock submission(m_submission);
        m_running = true;
        try
        {
            for (size_t index = 0; index < m_workers.size(); ++index)
            {
                auto *worker = m_workers[index].get();
                worker->thread = std::thread([this, worker, index] { consume(*worker, index); });
            }
        }
        // Thread creation can fail after some consumers have started. Clear the
        // running flag before unwinding so those consumers can exit during cleanup.
        catch (...)
        {
            m_running = false;
            throw;
        }
        return true;
    }
    // The submission guard has unwound before cleanup joins any workers.
    // Keeping that guard while joining could block workers inside failPool().
    catch (const std::exception &exception)
    {
        m_running = false;
        for (auto &worker : m_workers)
        {
            if (worker->thread.joinable())
            {
                worker->thread.join();
            }
            if (worker->app)
            {
                worker->app->terminate();
            }
        }
        m_workers.clear();
        if (error)
        {
            *error = exception.what();
        }
        return false;
    }
}

void ExecPool::stop()
{
    // Serialize lifecycle changes so worker ownership remains stable.
    std::scoped_lock lifecycle(m_lifecycle);
    // Close admission first. Workers observe the flag in their polling loops;
    // joining them below waits for active exchanges and child cleanup to finish.
    failPool();
    for (auto &worker : m_workers)
    {
        if (worker->thread.joinable())
        {
            worker->thread.join();
        }
    }
    m_workers.clear();
    drain();
}

void ExecPool::failPool()
{
    // Prevent a submission from entering the queue after shutdown drains it.
    std::scoped_lock lock(m_submission);
    m_running = false;
    drain();
}

void ExecPool::drain()
{
    // Reclaim queue wrappers; shared state survives while callers or workers hold it.
    while (auto *item = m_queue.pop(0))
    {
        std::unique_ptr<std::shared_ptr<State>> owned(item);
        finish(*owned, Error::STOPPED);
    }
}

bool ExecPool::isRunning() const noexcept
{
    return m_running;
}

ExecPool::Statistics ExecPool::statistics() const
{
    std::scoped_lock lock(m_statisticsMutex);
    // Copy counters together, then sample the independently synchronized queue.
    // Consumers can change queue occupancy without holding the statistics mutex.
    auto result = m_statistics;
    result.queued = m_queue.size();
    return result;
}
#endif
