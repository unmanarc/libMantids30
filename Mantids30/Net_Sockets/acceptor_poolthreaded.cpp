#include "acceptor_poolthreaded.h"
#include <memory>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets::Acceptors;

void PoolThreaded::init()
{
    setThreadRunner(runner, this);
    setThreadStopper(stopper, this);
}

PoolThreaded::PoolThreaded()
{
    init();
}

PoolThreaded::~PoolThreaded()
{
}

void PoolThreaded::run()
{
    std::unique_lock<std::mutex> lock(this->m_runMutex);

    // Create pool as local variable (lifetime contained within run())
    auto pool = std::make_unique<Mantids30::Threads::Pool::ThreadPool>(parameters.threadsCount, parameters.taskQueues);
    pool->start();

    m_running = true;

    // Create one thread per acceptor socket using lambda
    // Each thread runs its own blocking acceptConnection() loop
    for (auto &acceptorSocket : m_acceptorSocketList)
    {
        if (!acceptorSocket)
            continue;

        // Capture raw pointer to pool (safe: pool outlives all threads due to join() below)
        m_acceptorThreads.emplace_back(
            [this, poolPtr = pool.get(), acceptorSocket]()
            {
#ifndef _WIN32
                pthread_setname_np(pthread_self(), "acc:sck");
#endif
                while (this->m_running && acceptorSocket)
                {
                    std::shared_ptr<Sockets::Socket_Stream> clientSocket = acceptorSocket->acceptConnection();
                    if (clientSocket)
                    {
                        std::shared_ptr<sAcceptorTaskData> taskData = std::make_shared<sAcceptorTaskData>();

                        if (this->parameters.debugOptions.enabled)
                        {
                            uint32_t debugOptions = Socket_Stream::SOCKET_DEBUG_PRINT_CLOSE | Socket_Stream::SOCKET_DEBUG_PRINT_ERRORS;

                            if (this->parameters.debugOptions.printHex)
                                debugOptions |= Socket_Stream::SOCKET_DEBUG_PRINT_WRITE_HEX | Socket_Stream::SOCKET_DEBUG_PRINT_READ_HEX;

                            if (this->parameters.debugOptions.printPlainText)
                                debugOptions |= Socket_Stream::SOCKET_DEBUG_PRINT_READ_PLAIN | Socket_Stream::SOCKET_DEBUG_PRINT_WRITE_PLAIN;

                            clientSocket->setDebugOptions(debugOptions);
                            clientSocket->setDebugOutput(this->parameters.debugOptions.dir);
                        }

                        taskData->onConnect = this->callbacks.onClientConnected;
                        taskData->onInitFail = this->callbacks.onProtocolInitializationFailure;
                        taskData->contextOnConnect = this->callbacks.contextOnConnect;
                        taskData->contextOnInitFail = this->callbacks.contextOnInitFail;
                        taskData->clientSocket = clientSocket;
                        taskData->key = clientSocket->getRemotePairStr();

                        if (!poolPtr->pushTask(&acceptorTask, taskData, this->parameters.timeoutMS, this->parameters.queuesKeyRatio, taskData->key))
                        {
                            if (this->callbacks.onClientAcceptTimeoutOccurred != nullptr)
                                this->callbacks.onClientAcceptTimeoutOccurred(this->callbacks.contextOnTimedOut, clientSocket);
                        }
                    }
                }
            });
    }

    // Wait until stop is signaled (m_running set to false), then join all acceptor threads
    for (auto &t : m_acceptorThreads)
    {
        if (t.joinable())
            t.join();
    }

    m_acceptorThreads.clear();
    m_acceptorSocketList.clear();

    // Stop the pool (safe: all threads joined, no one uses poolPtr anymore)
    pool->stop();
}

void PoolThreaded::stop()
{
    m_running = false;

    for (auto &sock : m_acceptorSocketList)
    {
        if (sock)
            sock->shutdownSocket();
    }
}

void PoolThreaded::addAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream> &value)
{
    m_acceptorSocketList.push_back(value);
}

void PoolThreaded::runner(void *data)
{
    ((PoolThreaded *) data)->run();
}

void PoolThreaded::stopper(void *data)
{
    ((PoolThreaded *) data)->stop();
}

void PoolThreaded::acceptorTask(std::shared_ptr<void> data)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "poolthr:sckacpt");
#endif
    sAcceptorTaskData *taskData = (sAcceptorTaskData *) data.get();
    if (taskData->clientSocket->postAcceptSubInitialization())
    {
        // Start
        if (taskData->onConnect)
        {
            taskData->onConnect(taskData->contextOnConnect, taskData->clientSocket);
        }
    }
    else
    {
        if (taskData->onInitFail)
        {
            taskData->onInitFail(taskData->contextOnInitFail, taskData->clientSocket);
        }
    }
}
