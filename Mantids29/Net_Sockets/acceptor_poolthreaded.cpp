#include "acceptor_poolthreaded.h"
#include <string.h>

using namespace Mantids29::Network;
using namespace Mantids29::Network::Sockets::Acceptors;

void PoolThreaded::init()
{
    this->pool = nullptr;
    this->acceptorSocket = nullptr;

    onConnect = nullptr;
    onInitFail = nullptr;
    onTimedOut = nullptr;

    objOnConnect = nullptr;
    objOnInitFail = nullptr;
    objOnTimedOut = nullptr;

    setThreadRunner(runner,this);
    setThreadStopper(stopper,this);

    setThreadsCount(52);
    setTaskQueues(36);
    setTimeoutMS(5000);
    setQueuesKeyRatio(0.5);
}


PoolThreaded::PoolThreaded()
{
    init();
}

PoolThreaded::PoolThreaded(const std::shared_ptr<Sockets::Socket_Stream_Base> & acceptorSocket, _callbackConnectionRB _onConnect, void *obj, _callbackConnectionRB _onInitFailed, _callbackConnectionRV _onTimeOut)
{
    init();
    setAcceptorSocket(acceptorSocket);
    setCallbackOnConnect(_onConnect,obj);
    setCallbackOnInitFail(_onInitFailed,obj);
    setCallbackOnTimedOut(_onTimeOut,obj);
}

void PoolThreaded::setCallbackOnConnect(_callbackConnectionRB _onConnect, void *obj)
{
    this->onConnect = _onConnect;
    this->objOnConnect = obj;
}

void PoolThreaded::setCallbackOnInitFail(_callbackConnectionRB _onInitFailed, void *obj)
{
    this->onInitFail = _onInitFailed;
    this->objOnInitFail = obj;
}

void PoolThreaded::setCallbackOnTimedOut(_callbackConnectionRV _onTimeOut, void *obj)
{
    this->onTimedOut = _onTimeOut;
    this->objOnTimedOut = obj;
}


PoolThreaded::~PoolThreaded()
{
    if (this->pool)
        delete this->pool;
}

void PoolThreaded::run()
{
    this->pool = new Mantids29::Threads::Pool::ThreadPool(threadsCount,taskQueues);
    pool->start();
    for(;;)
    {
        Sockets::Socket_Stream_Base * clientSocket = acceptorSocket->acceptConnection();
        if (clientSocket)
        {
            sAcceptorTaskData * taskData = new sAcceptorTaskData;
            clientSocket->getRemotePair(taskData->remotePair);
            taskData->onConnect = onConnect;
            taskData->onInitFail = onInitFail;
            taskData->objOnConnect = objOnConnect;
            taskData->objOnInitFail = objOnInitFail;
            taskData->clientSocket = clientSocket;
            taskData->isSecure = acceptorSocket->isSecure();

            taskData->key = taskData->remotePair;

            if (!pool->pushTask( &acceptorTask, taskData, timeoutMS, queuesKeyRatio, taskData->key))
            {
                if (onTimedOut!=nullptr)
                    onTimedOut(objOnTimedOut,clientSocket,taskData->remotePair,acceptorSocket->isSecure());
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
            if (!taskData->onConnect(taskData->objOnConnect, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (taskData->onInitFail)
        {
            if (!taskData->onInitFail(taskData->objOnInitFail, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    delete taskData;
}
