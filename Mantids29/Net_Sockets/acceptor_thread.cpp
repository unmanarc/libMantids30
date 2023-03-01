#include "acceptor_thread.h"

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

#include <string.h>

#include "acceptor_multithreaded.h"
#include <Mantids29/Helpers/mem.h>

using namespace Mantids29::Network::Sockets::Acceptors;

SAThread::SAThread()
{
    ZeroBArray(m_remotePair);

    m_onConnect = nullptr;
    m_onInitFail = nullptr;

    m_objectOnConnect = nullptr;
    m_objectOnInitFail = nullptr;
    m_parent = nullptr;
    m_pClientSocket = nullptr;
}

SAThread::~SAThread()
{
    if (m_pClientSocket)
    {
        delete m_pClientSocket;
        m_pClientSocket = nullptr;
    }
}

void SAThread::start()
{
    std::thread(thread_streamclient,this,(MultiThreaded *)m_parent).detach();
}

void SAThread::stopSocket()
{
    m_pClientSocket->shutdownSocket();
}

void SAThread::setCallbackOnConnect(bool(*_onConnect)(void *, Sockets::Socket_Stream_Base *, const char *, bool), void *objectOnConnected)
{
    this->m_onConnect = _onConnect;
    this->m_objectOnConnect = objectOnConnected;
}

void SAThread::setCallbackOnInitFail(bool (*_onInitFailed)(void *, Sockets::Socket_Stream_Base *, const char *, bool), void *objectOnInitFailed)
{
    this->m_onInitFail = _onInitFailed;
    this->m_objectOnInitFail = objectOnInitFailed;
}


void SAThread::setParent(void *parent)
{
    this->m_parent = parent;
}

void SAThread::postInitConnection()
{
    // Accept (internal protocol)
    if (m_pClientSocket->postAcceptSubInitialization())
    {
        // Start
        if (m_onConnect)
        {
            if (!this->m_onConnect(m_objectOnConnect, m_pClientSocket, m_remotePair,m_isSecure))
            {
                m_pClientSocket = nullptr;
            }
        }
    }
    else
    {
        if (m_onInitFail)
        {
            if (!this->m_onInitFail(m_objectOnInitFail, m_pClientSocket, m_remotePair,m_isSecure))
            {
                m_pClientSocket = nullptr;
            }
        }
    }
}

void SAThread::setClientSocket(Sockets::Socket_Stream_Base * _clientSocket)
{
    m_pClientSocket = _clientSocket;
    m_pClientSocket->getRemotePair(m_remotePair);
}

const char *SAThread::getRemotePair()
{
    return m_remotePair;
}

void SAThread::thread_streamclient(SAThread *threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((MultiThreaded *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}

bool SAThread::isSecure() const
{
    return m_isSecure;
}

void SAThread::setSecure(bool value)
{
    m_isSecure = value;
}
