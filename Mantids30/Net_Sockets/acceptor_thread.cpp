#include "acceptor_thread.h"
#include <memory>

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

#include "acceptor_multithreaded.h"
#include <Mantids30/Helpers/mem.h>

using namespace Mantids30::Network::Sockets::Acceptors;

SAThread::SAThread()
{
    //ZeroBArray(m_remotePair);

    m_pClientSocket = nullptr;
}

SAThread::~SAThread()
{
}

void SAThread::stopSocket()
{
    m_pClientSocket->shutdownSocket();
}

void SAThread::postInitConnection()
{
    // Accept (internal protocol)
    if (m_pClientSocket->postAcceptSubInitialization())
    {
        // Start
        if (callbacks.onClientConnected)
        {
            if (!this->callbacks.onClientConnected(callbacks.contextOnConnect, m_pClientSocket))
            {
                m_pClientSocket = nullptr;
            }
        }
    }
    else
    {
        if (callbacks.onProtocolInitializationFailure)
        {
            if (!this->callbacks.onProtocolInitializationFailure(callbacks.contextOnInitFail, m_pClientSocket))
            {
                m_pClientSocket = nullptr;
            }
        }
    }
}

void SAThread::setClientSocket(std::shared_ptr<Sockets::Socket_Stream_Base> _clientSocket)
{
    m_pClientSocket = _clientSocket;
//    m_pClientSocket->getRemotePair(m_remotePair);
}

std::string SAThread::getRemotePair() const
{
    return m_pClientSocket->getRemotePairStr();
}

/*
const char *SAThread::getRemotePair()
{
    return m_remotePair;
}*/

void SAThread::thread_streamclient(std::shared_ptr<SAThread> threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((MultiThreaded *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}
/*
bool SAThread::isSecure() const
{
    return m_isSecure;
}

void SAThread::setSecure(bool value)
{
    m_isSecure = value;
}
*/
