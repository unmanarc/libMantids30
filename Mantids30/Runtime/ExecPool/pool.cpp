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
        logProblem(Error::INVALID_INPUT, "Invalid pool configuration");
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
        logProblem(Error::SPAWN_FAILED, "Pool startup failed", exception.what());
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
    // Shared state survives while callers or workers hold it.
    while (auto state = m_queue.pop(0))
    {
        finish(state, Error::STOPPED);
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

void ExecPool::logProblem(Error error, const char *context, const char *detail, size_t worker, int systemError) const noexcept
{
    if (!m_config.appLog || error == Error::SUCCESS || error == Error::STOPPED)
    {
        return;
    }
    const char *name = "INTERNAL_ERROR";
    switch (error)
    {
    case Error::NOT_RUNNING:
        name = "NOT_RUNNING";
        break;
    case Error::QUEUE_FULL:
        name = "QUEUE_FULL";
        break;
    case Error::TIMEOUT:
        name = "TIMEOUT";
        break;
    case Error::SPAWN_FAILED:
        name = "SPAWN_FAILED";
        break;
    case Error::PROCESS_EXITED:
        name = "PROCESS_EXITED";
        break;
    case Error::IO_ERROR:
        name = "IO_ERROR";
        break;
    case Error::INVALID_JSON:
        name = "INVALID_JSON";
        break;
    case Error::DESYNCHRONIZED:
        name = "DESYNCHRONIZED";
        break;
    case Error::LINE_TOO_LONG:
        name = "LINE_TOO_LONG";
        break;
    case Error::INVALID_INPUT:
        name = "INVALID_INPUT";
        break;
    case Error::INPUT_TOO_LONG:
        name = "INPUT_TOO_LONG";
        break;
    case Error::ALREADY_EXECUTED:
        name = "ALREADY_EXECUTED";
        break;
    case Error::INTERNAL_ERROR:
        name = "INTERNAL_ERROR";
        break;
    default:
        break;
    }
    const auto level = error == Error::NOT_RUNNING || error == Error::QUEUE_FULL || error == Error::TIMEOUT ||
        error == Error::INVALID_INPUT || error == Error::INPUT_TOO_LONG || error == Error::ALREADY_EXECUTED
        ? Logs::WARNING : Logs::ERROR;
    try
    {
        // AppLog serializes concurrent writers. Fixed format strings keep diagnostic
        // details literal, and no JSON input/output is exposed through this path.
        m_config.appLog->log0("ExecPool", level, "%s: %s (worker=%zu, errno=%d). %s", context, name, worker, systemError, detail);
    }
    catch (...)
    {
        // Diagnostics are best effort, including from noexcept worker cleanup paths.
    }
}
#endif
