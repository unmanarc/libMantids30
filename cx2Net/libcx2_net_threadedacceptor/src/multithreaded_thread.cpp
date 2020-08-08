#include "multithreaded_thread.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#include "multithreaded_acceptor.h"


using namespace CX2::Network::Streams::ThreadedAcceptors;


MultiThreaded_Accepted_Thread::MultiThreaded_Accepted_Thread()
{
    memset(remotePair,0,INET6_ADDRSTRLEN+2);

    callbackOnConnect = nullptr;
    callbackOnInitFail = nullptr;

    objOnConnect = nullptr;
    objOnInitFail = nullptr;
    parent = nullptr;
    clientSocket = nullptr;
}

MultiThreaded_Accepted_Thread::~MultiThreaded_Accepted_Thread()
{
    if (clientSocket)
    {
        delete clientSocket;
        clientSocket = nullptr;
    }
}

void MultiThreaded_Accepted_Thread::start()
{
    std::thread(thread_streamclient,this,(MultiThreaded_Acceptor *)parent).detach();
}

void MultiThreaded_Accepted_Thread::stopSocket()
{
    clientSocket->shutdownSocket();
}

void MultiThreaded_Accepted_Thread::setCallbackOnConnect(bool(*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void MultiThreaded_Accepted_Thread::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}


void MultiThreaded_Accepted_Thread::setParent(void *parent)
{
    this->parent = parent;
}

void MultiThreaded_Accepted_Thread::postInitConnection()
{
    // Accept (internal protocol)
    if (clientSocket->postAcceptSubInitialization())
    {
        // Start
        if (callbackOnConnect)
        {
            if (!this->callbackOnConnect(objOnConnect, clientSocket, remotePair,isSecure))
            {
                clientSocket = nullptr;
            }
        }
    }
    else
    {
        if (callbackOnInitFail)
        {
            if (!this->callbackOnInitFail(objOnInitFail, clientSocket, remotePair,isSecure))
            {
                clientSocket = nullptr;
            }
        }
    }
}

void MultiThreaded_Accepted_Thread::setClientSocket(Streams::StreamSocket * _clientSocket)
{
    clientSocket = _clientSocket;
    clientSocket->getRemotePair(remotePair);
}

const char *MultiThreaded_Accepted_Thread::getRemotePair()
{
    return remotePair;
}

void MultiThreaded_Accepted_Thread::thread_streamclient(MultiThreaded_Accepted_Thread *threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((MultiThreaded_Acceptor *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}

bool MultiThreaded_Accepted_Thread::getIsSecure() const
{
    return isSecure;
}

void MultiThreaded_Accepted_Thread::setIsSecure(bool value)
{
    isSecure = value;
}
