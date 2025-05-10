#include "acceptor_poolthreaded.h"
#include <memory>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets::Acceptors;

void PoolThreaded::init()
{
    this->m_pool = nullptr;
    this->m_acceptorSocket = nullptr;

    setThreadRunner(runner,this);
    setThreadStopper(stopper,this);
}


PoolThreaded::PoolThreaded()
{
    init();
}

PoolThreaded::PoolThreaded(const std::shared_ptr<Sockets::Socket_Stream> & acceptorSocket, _callbackConnectionRB _onConnect, void * context, _callbackConnectionRB _onInitFailed, _callbackConnectionRV _onTimeOut)
{
    init();
    setAcceptorSocket(acceptorSocket);

    callbacks.setAllContexts( context );
    callbacks.onClientConnected = _onConnect;
    callbacks.onProtocolInitializationFailure = _onInitFailed;
    callbacks.onClientAcceptTimeoutOccurred = _onTimeOut;
}


PoolThreaded::~PoolThreaded()
{
    if (this->m_pool)
        delete this->m_pool;
}

void PoolThreaded::run()
{
    std::unique_lock<std::mutex> lock(this->m_runMutex);

    this->m_pool = new Mantids30::Threads::Pool::ThreadPool(parameters.threadsCount,parameters.taskQueues);
    m_pool->start();

    for(;;)
    {
        std::shared_ptr<Sockets::Socket_Stream> clientSocket = m_acceptorSocket->acceptConnection();
        if (clientSocket)
        {
            // TODO: shared_ptr
            std::shared_ptr<sAcceptorTaskData> taskData = std::make_shared<sAcceptorTaskData>();

            taskData->onConnect = callbacks.onClientConnected;
            taskData->onInitFail = callbacks.onProtocolInitializationFailure;
            taskData->contextOnConnect = callbacks.contextOnConnect;
            taskData->contextOnInitFail = callbacks.contextOnInitFail;
            taskData->clientSocket = clientSocket;

            taskData->key = clientSocket->getRemotePairStr();

            if (!m_pool->pushTask( &acceptorTask, taskData, parameters.timeoutMS, parameters.queuesKeyRatio, taskData->key))
            {
                if (callbacks.onClientAcceptTimeoutOccurred!=nullptr)
                    callbacks.onClientAcceptTimeoutOccurred(callbacks.contextOnTimedOut,clientSocket);
            }
        }
        else
            break;
    }

    // Destroy the acceptor socket:
    m_acceptorSocket = nullptr;
}

void PoolThreaded::stop()
{
    m_acceptorSocket->shutdownSocket();
}

void PoolThreaded::setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream> & value)
{
    m_acceptorSocket = value;
}

void PoolThreaded::runner(void *data)
{
    ((PoolThreaded *)data)->run();
}

void PoolThreaded::stopper(void *data)
{
    ((PoolThreaded *)data)->stop();
}

void PoolThreaded::acceptorTask(std::shared_ptr<void> data)
{
#ifndef _WIN32
     pthread_setname_np(pthread_self(), "poolthr:sckacpt");
#endif

     sAcceptorTaskData * taskData = (sAcceptorTaskData *)data.get();
    if (taskData->clientSocket->postAcceptSubInitialization())
    {
        // Start
        if (taskData->onConnect)
        {
            if (!taskData->onConnect(taskData->contextOnConnect, taskData->clientSocket))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (taskData->onInitFail)
        {
            if (!taskData->onInitFail(taskData->contextOnInitFail, taskData->clientSocket))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
}
