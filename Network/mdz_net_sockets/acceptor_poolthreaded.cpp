#include "acceptor_poolthreaded.h"
#include <string.h>

using namespace Mantids::Network;
using namespace Mantids::Network::Sockets::Acceptors;

void PoolThreaded::init()
{
    this->pool = nullptr;
    this->acceptorSocket = nullptr;

    callbackOnConnect = nullptr;
    callbackOnInitFail = nullptr;
    callbackOnTimedOut = nullptr;

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

PoolThreaded::PoolThreaded(Socket_StreamBase *acceptorSocket, _callbackConnectionRB _callbackOnConnect, void *obj, _callbackConnectionRB _callbackOnInitFailed, _callbackConnectionRV _callbackOnTimeOut)
{
    init();
    setAcceptorSocket(acceptorSocket);
    setCallbackOnConnect(_callbackOnConnect,obj);
    setCallbackOnInitFail(_callbackOnInitFailed,obj);
    setCallbackOnTimedOut(_callbackOnTimeOut,obj);
}

void PoolThreaded::setCallbackOnConnect(_callbackConnectionRB _callbackOnConnect, void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void PoolThreaded::setCallbackOnInitFail(_callbackConnectionRB _callbackOnInitFailed, void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}

void PoolThreaded::setCallbackOnTimedOut(_callbackConnectionRV _callbackOnTimeOut, void *obj)
{
    this->callbackOnTimedOut = _callbackOnTimeOut;
    this->objOnTimedOut = obj;
}


PoolThreaded::~PoolThreaded()
{
    if (this->pool)
        delete this->pool;
    if (this->acceptorSocket)
        delete acceptorSocket;
}

void PoolThreaded::run()
{
    this->pool = new Mantids::Threads::Pool::ThreadPool(threadsCount,taskQueues);
    pool->start();
    for(;;)
    {
        Sockets::Socket_StreamBase * clientSocket = acceptorSocket->acceptConnection();
        if (clientSocket)
        {
            sAcceptorTaskData * taskData = new sAcceptorTaskData;
            clientSocket->getRemotePair(taskData->remotePair);
            taskData->callbackOnConnect = callbackOnConnect;
            taskData->callbackOnInitFail = callbackOnInitFail;
            taskData->objOnConnect = objOnConnect;
            taskData->objOnInitFail = objOnInitFail;
            taskData->clientSocket = clientSocket;
            taskData->isSecure = acceptorSocket->isSecure();

            taskData->key = taskData->remotePair;

            if (!pool->pushTask( &acceptorTask, taskData, timeoutMS, queuesKeyRatio, taskData->key))
            {
                if (callbackOnTimedOut!=nullptr)
                    callbackOnTimedOut(objOnTimedOut,clientSocket,taskData->remotePair,acceptorSocket->isSecure());
                delete taskData;
            }
        }
        else
            break;
    }
    delete acceptorSocket;
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

void PoolThreaded::setAcceptorSocket(Sockets::Socket_StreamBase *value)
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
        if (taskData->callbackOnConnect)
        {
            if (!taskData->callbackOnConnect(taskData->objOnConnect, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (taskData->callbackOnInitFail)
        {
            if (!taskData->callbackOnInitFail(taskData->objOnInitFail, taskData->clientSocket, taskData->remotePair, taskData->isSecure))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    delete taskData;
}
