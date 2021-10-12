#include "socket_acceptor_thread.h"

#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

#include <string.h>

#include "socket_acceptor_multithreaded.h"
#include <cx2_hlp_functions/mem.h>

using namespace CX2::Network::Sockets::Acceptors;

Socket_Acceptor_Thread::Socket_Acceptor_Thread()
{
    ZeroBArray(remotePair);

    callbackOnConnect = nullptr;
    callbackOnInitFail = nullptr;

    objOnConnect = nullptr;
    objOnInitFail = nullptr;
    parent = nullptr;
    clientSocket = nullptr;
}

Socket_Acceptor_Thread::~Socket_Acceptor_Thread()
{
    if (clientSocket)
    {
        delete clientSocket;
        clientSocket = nullptr;
    }
}

void Socket_Acceptor_Thread::start()
{
    std::thread(thread_streamclient,this,(Socket_Acceptor_MultiThreaded *)parent).detach();
}

void Socket_Acceptor_Thread::stopSocket()
{
    clientSocket->shutdownSocket();
}

void Socket_Acceptor_Thread::setCallbackOnConnect(bool(*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void Socket_Acceptor_Thread::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}


void Socket_Acceptor_Thread::setParent(void *parent)
{
    this->parent = parent;
}

void Socket_Acceptor_Thread::postInitConnection()
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

void Socket_Acceptor_Thread::setClientSocket(Streams::StreamSocket * _clientSocket)
{
    clientSocket = _clientSocket;
    clientSocket->getRemotePair(remotePair);
}

const char *Socket_Acceptor_Thread::getRemotePair()
{
    return remotePair;
}

void Socket_Acceptor_Thread::thread_streamclient(Socket_Acceptor_Thread *threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((Socket_Acceptor_MultiThreaded *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}

bool Socket_Acceptor_Thread::getIsSecure() const
{
    return isSecure;
}

void Socket_Acceptor_Thread::setIsSecure(bool value)
{
    isSecure = value;
}
