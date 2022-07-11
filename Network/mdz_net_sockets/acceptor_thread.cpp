#include "acceptor_thread.h"

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

#include <string.h>

#include "acceptor_multithreaded.h"
#include <mdz_hlp_functions/mem.h>

using namespace Mantids::Network::Sockets::Acceptors;

SAThread::SAThread()
{
    ZeroBArray(remotePair);

    callbackOnConnect = nullptr;
    callbackOnInitFail = nullptr;

    objOnConnect = nullptr;
    objOnInitFail = nullptr;
    parent = nullptr;
    clientSocket = nullptr;
}

SAThread::~SAThread()
{
    if (clientSocket)
    {
        delete clientSocket;
        clientSocket = nullptr;
    }
}

void SAThread::start()
{
    std::thread(thread_streamclient,this,(MultiThreaded *)parent).detach();
}

void SAThread::stopSocket()
{
    clientSocket->shutdownSocket();
}

void SAThread::setCallbackOnConnect(bool(*_callbackOnConnect)(void *, Sockets::Socket_StreamBase *, const char *, bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void SAThread::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Sockets::Socket_StreamBase *, const char *, bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}


void SAThread::setParent(void *parent)
{
    this->parent = parent;
}

void SAThread::postInitConnection()
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

void SAThread::setClientSocket(Sockets::Socket_StreamBase * _clientSocket)
{
    clientSocket = _clientSocket;
    clientSocket->getRemotePair(remotePair);
}

const char *SAThread::getRemotePair()
{
    return remotePair;
}

void SAThread::thread_streamclient(SAThread *threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((MultiThreaded *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}

bool SAThread::getIsSecure() const
{
    return isSecure;
}

void SAThread::setIsSecure(bool value)
{
    isSecure = value;
}
