#include "socket_acceptor_poolthreaded.h"
#include <string.h>
#include <cx2_hlp_functions/mem.h>

namespace CX2 { namespace Network { namespace Sockets { namespace Acceptors {

struct sAcceptorTaskData
{
    ~sAcceptorTaskData()
    {
        if (clientSocket)
        {
            clientSocket->shutdownSocket();
            delete clientSocket;
            clientSocket = nullptr;
        }
        isSecure = false;
        ZeroBArray(remotePair);
    }

    bool (*callbackOnConnect)(void *,Streams::StreamSocket *, const char *,bool);
    bool (*callbackOnInitFail)(void *,Streams::StreamSocket *, const char *,bool);
    void *objOnConnect, *objOnInitFail;

    std::string key;

    void * obj;
    Streams::StreamSocket * clientSocket;
    char remotePair[INET6_ADDRSTRLEN];
    bool isSecure;
    char pad;
};

}}}}

using namespace CX2::Network;
using namespace CX2::Network::Sockets::Acceptors;


Socket_Acceptor_PoolThreaded::Socket_Acceptor_PoolThreaded()
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

void Socket_Acceptor_PoolThreaded::setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *,bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void Socket_Acceptor_PoolThreaded::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *,bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}

void Socket_Acceptor_PoolThreaded::setCallbackOnTimedOut(void (*_callbackOnTimeOut)(void *, Streams::StreamSocket *, const char *,bool), void *obj)
{
    this->callbackOnTimedOut = _callbackOnTimeOut;
    this->objOnTimedOut = obj;
}


Socket_Acceptor_PoolThreaded::~Socket_Acceptor_PoolThreaded()
{
    if (this->pool) delete this->pool;
    if (this->acceptorSocket) delete acceptorSocket;
}

void Socket_Acceptor_PoolThreaded::run()
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

void Socket_Acceptor_PoolThreaded::stop()
{
    acceptorSocket->shutdownSocket();
}

uint32_t Socket_Acceptor_PoolThreaded::getTimeoutMS() const
{
    return timeoutMS;
}

void Socket_Acceptor_PoolThreaded::setTimeoutMS(const uint32_t &value)
{
    timeoutMS = value;
}

uint32_t Socket_Acceptor_PoolThreaded::getThreadsCount() const
{
    return threadsCount;
}

void Socket_Acceptor_PoolThreaded::setThreadsCount(const uint32_t &value)
{
    threadsCount = value;
}

uint32_t Socket_Acceptor_PoolThreaded::getTaskQueues() const
{
    return taskQueues;
}

void Socket_Acceptor_PoolThreaded::setTaskQueues(const uint32_t &value)
{
    taskQueues = value;
}

float Socket_Acceptor_PoolThreaded::getQueuesKeyRatio() const
{
    return queuesKeyRatio;
}

void Socket_Acceptor_PoolThreaded::setQueuesKeyRatio(float value)
{
    queuesKeyRatio = value;
}

void Socket_Acceptor_PoolThreaded::setAcceptorSocket(Streams::StreamSocket *value)
{
    acceptorSocket = value;
}

void Socket_Acceptor_PoolThreaded::runner(void *data)
{
    ((Socket_Acceptor_PoolThreaded *)data)->run();
}

void Socket_Acceptor_PoolThreaded::stopper(void *data)
{
    ((Socket_Acceptor_PoolThreaded *)data)->stop();
}

void Socket_Acceptor_PoolThreaded::acceptorTask(void *data)
{
#ifndef _WIN32
     pthread_setname_np(pthread_self(), "poolthr_sockaccept");
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
