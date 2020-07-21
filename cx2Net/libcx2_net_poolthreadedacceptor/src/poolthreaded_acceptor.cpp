#include "poolthreaded_acceptor.h"
#include <string.h>

namespace CX2 { namespace Network { namespace Streams { namespace ThreadedAcceptors {

struct sAcceptorTaskData
{
    ~sAcceptorTaskData()
    {
        memset(remotePair,0,INET6_ADDRSTRLEN+1);
        if (clientSocket)
        {
            clientSocket->shutdownSocket();
            delete clientSocket;
            clientSocket = nullptr;
        }
    }

    bool (*callbackOnConnect)(void *,Streams::StreamSocket *, const char *);
    bool (*callbackOnInitFail)(void *,Streams::StreamSocket *, const char *);
    void *objOnConnect, *objOnInitFail;

    std::string key;

    void * obj;
    Streams::StreamSocket * clientSocket;
    char remotePair[INET6_ADDRSTRLEN+2];
    char pad;
};

}}}}

using namespace CX2::Network;
using namespace CX2::Network::Streams::ThreadedAcceptors;


PoolThreaded_Acceptor::PoolThreaded_Acceptor()
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

void PoolThreaded_Acceptor::setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void PoolThreaded_Acceptor::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}

void PoolThreaded_Acceptor::setCallbackOnTimedOut(void (*_callbackOnTimedOut)(void *, Streams::StreamSocket *, const char *), void *obj)
{
    this->callbackOnTimedOut = _callbackOnTimedOut;
    this->objOnTimedOut = obj;
}


PoolThreaded_Acceptor::~PoolThreaded_Acceptor()
{
    if (this->pool) delete this->pool;
    if (this->acceptorSocket) delete acceptorSocket;
}

void PoolThreaded_Acceptor::run()
{
    this->pool = new CX2::Threads::Pool::ThreadPool(threadsCount,taskQueues);
    pool->start();
    for(;;)
    {
        Streams::StreamSocket * clientSocket = acceptorSocket->acceptConnection();
        if (clientSocket)
        {
            sAcceptorTaskData * taskData = new sAcceptorTaskData;
            clientSocket->getRemotePair(taskData->remotePair);
            taskData->callbackOnConnect = callbackOnConnect;
            taskData->callbackOnInitFail = callbackOnInitFail;
            taskData->objOnConnect = objOnConnect;
            taskData->objOnInitFail = objOnInitFail;
            taskData->clientSocket = clientSocket;

            taskData->key = taskData->remotePair;

            if (!pool->pushTask( &acceptorTask, taskData, timeoutMS, queuesKeyRatio, taskData->key))
            {
                if (callbackOnTimedOut!=nullptr) callbackOnTimedOut(objOnTimedOut,clientSocket,taskData->remotePair);
                delete taskData;
            }
        }
        else
            break;
    }
    delete acceptorSocket;
}

void PoolThreaded_Acceptor::stop()
{
    acceptorSocket->shutdownSocket();
}

uint32_t PoolThreaded_Acceptor::getTimeoutMS() const
{
    return timeoutMS;
}

void PoolThreaded_Acceptor::setTimeoutMS(const uint32_t &value)
{
    timeoutMS = value;
}

uint32_t PoolThreaded_Acceptor::getThreadsCount() const
{
    return threadsCount;
}

void PoolThreaded_Acceptor::setThreadsCount(const uint32_t &value)
{
    threadsCount = value;
}

uint32_t PoolThreaded_Acceptor::getTaskQueues() const
{
    return taskQueues;
}

void PoolThreaded_Acceptor::setTaskQueues(const uint32_t &value)
{
    taskQueues = value;
}

float PoolThreaded_Acceptor::getQueuesKeyRatio() const
{
    return queuesKeyRatio;
}

void PoolThreaded_Acceptor::setQueuesKeyRatio(float value)
{
    queuesKeyRatio = value;
}

void PoolThreaded_Acceptor::setAcceptorSocket(Streams::StreamSocket *value)
{
    acceptorSocket = value;
}

void PoolThreaded_Acceptor::runner(void *data)
{
    ((PoolThreaded_Acceptor *)data)->run();
}

void PoolThreaded_Acceptor::stopper(void *data)
{
    ((PoolThreaded_Acceptor *)data)->stop();
}

void PoolThreaded_Acceptor::acceptorTask(void *data)
{
    sAcceptorTaskData * taskData = ((sAcceptorTaskData *)data);
    if (taskData->clientSocket->postAcceptSubInitialization())
    {
        // Start
        if (taskData->callbackOnConnect)
        {
            if (!taskData->callbackOnConnect(taskData->objOnConnect, taskData->clientSocket, taskData->remotePair))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (taskData->callbackOnInitFail)
        {
            if (!taskData->callbackOnInitFail(taskData->objOnInitFail, taskData->clientSocket, taskData->remotePair))
            {
                taskData->clientSocket = nullptr;
            }
        }
    }
    delete taskData;
}
