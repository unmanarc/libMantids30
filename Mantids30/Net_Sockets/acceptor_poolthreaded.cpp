#include "acceptor_poolthreaded.h"
#include <memory>
#include <string.h>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets::Acceptors;

void PoolThreaded::init()
{
    this->pool = nullptr;
    this->acceptorSocket = nullptr;


    setThreadRunner(runner,this);
    setThreadStopper(stopper,this);

}


PoolThreaded::PoolThreaded()
{
    init();
}

PoolThreaded::PoolThreaded(const std::shared_ptr<Sockets::Socket_Stream_Base> & acceptorSocket, _callbackConnectionRB _onConnect, std::shared_ptr<void> context, _callbackConnectionRB _onInitFailed, _callbackConnectionRV _onTimeOut)
{
    init();
    setAcceptorSocket(acceptorSocket);
    setCallbackOnConnect(_onConnect,context);
    setCallbackOnInitFail(_onInitFailed,context);
    setCallbackOnTimedOut(_onTimeOut,context);
}

void PoolThreaded::setCallbackOnConnect(_callbackConnectionRB _onConnect, std::shared_ptr<void> context)
{
    this->onConnect = _onConnect;
    this->contextOnConnect = context;
}

void PoolThreaded::setCallbackOnInitFail(_callbackConnectionRB _onInitFailed, std::shared_ptr<void> context)
{
    this->onInitFail = _onInitFailed;
    this->contextOnInitFail = context;
}

void PoolThreaded::setCallbackOnTimedOut(_callbackConnectionRV _onTimeOut, std::shared_ptr<void> context)
{
    this->onTimedOut = _onTimeOut;
    this->contextOnTimedOut = context;
}


PoolThreaded::~PoolThreaded()
{
    if (this->pool)
        delete this->pool;
}

void PoolThreaded::run()
{
    this->pool = new Mantids30::Threads::Pool::ThreadPool(threadsCount,taskQueues);
    pool->start();
    for(;;)
    {
        std::shared_ptr<Sockets::Socket_Stream_Base> clientSocket = acceptorSocket->acceptConnection();
        if (clientSocket)
        {
            sAcceptorTaskData * taskData = new sAcceptorTaskData;
            clientSocket->getRemotePair(taskData->remotePair);
            taskData->onConnect = onConnect;
            taskData->onInitFail = onInitFail;
            taskData->contextOnConnect = contextOnConnect;
            taskData->contextOnInitFail = contextOnInitFail;
            taskData->clientSocket = clientSocket;
            taskData->isSecure = acceptorSocket->isSecure();

            taskData->key = taskData->remotePair;

            if (!pool->pushTask( &acceptorTask, taskData, timeoutMS, queuesKeyRatio, taskData->key))
            {
                if (onTimedOut!=nullptr)
                    onTimedOut(contextOnTimedOut,clientSocket,taskData->remotePair,acceptorSocket->isSecure());
                delete taskData;
            }
        }
        else
            break;
    }
    // Destroy the acceptor socket:
    acceptorSocket = nullptr;
}

void PoolThreaded::stop()
{
    acceptorSocket->shutdownSocket();
}

uint32_t PoolThreaded::getTimeoutMS() const
{
    return timeoutMS;
}

void PoolThreaded::setTimeoutMS(const uint32_t &value)
{
    timeoutMS = value;
}

uint32_t PoolThreaded::getThreadsCount() const
{
    return threadsCount;
}

void PoolThreaded::setThreadsCount(const uint32_t &value)
{
    threadsCount = value;
}

uint32_t PoolThreaded::getTaskQueues() const
{
    return taskQueues;
}

void PoolThreaded::setTaskQueues(const uint32_t &value)
{
    taskQueues = value;
}

float PoolThreaded::getQueuesKeyRatio() const
{
    return queuesKeyRatio;
}

void PoolThreaded::setQueuesKeyRatio(float value)
{
    queuesKeyRatio = value;
}

void PoolThreaded::setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream_Base> & value)
{
    acceptorSocket = value;
}

void PoolThreaded::runner(void *data)
{
    ((PoolThreaded *)data)->run();
}

void PoolThreaded::stopper(void *data)
{
    ((PoolThreaded *)data)->stop();
}

void PoolThreaded::acceptorTask(void *data)
{
#ifndef _WIN32
     pthread_setname_np(pthread_self(), "poolthr:sckacpt");
#endif

    sAcceptorTaskData * taskData = ((sAcceptorTaskData *)data);
    if (taskData->clientSocket->postAcceptSubInitialization())
    {
        // Start
        if (taskData->onConnect)
        {
            if (!taskData->onConnect(taskData->contextOnConnect, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (taskData->onInitFail)
        {
            if (!taskData->onInitFail(taskData->contextOnInitFail, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    delete taskData;
}
