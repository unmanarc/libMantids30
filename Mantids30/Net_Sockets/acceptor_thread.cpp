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

void StreamAcceptorThread::stopSocket()
{
    m_pClientSocket->shutdownSocket();
}

void StreamAcceptorThread::postInitConnection()
{
    // Accept (internal protocol)
    if (m_pClientSocket->postAcceptSubInitialization())
    {
        // Start
        if (callbacks.onClientConnected)
        {
            this->callbacks.onClientConnected(callbacks.contextOnConnect, m_pClientSocket);
        }
    }
    else
    {
        if (callbacks.onProtocolInitializationFailure)
        {
            this->callbacks.onProtocolInitializationFailure(callbacks.contextOnInitFail, m_pClientSocket);
        }
    }
}

void StreamAcceptorThread::setClientSocket(std::shared_ptr<Sockets::Socket_Stream> _clientSocket)
{
    m_pClientSocket = _clientSocket;
}

std::string StreamAcceptorThread::getRemotePair() const
{
    return m_pClientSocket->getRemotePairStr();
}

void StreamAcceptorThread::thread_streamclient(std::shared_ptr<StreamAcceptorThread> threadClient, void *threadedAcceptedControl)
{
    threadClient->postInitConnection();
    ((MultiThreaded *)threadedAcceptedControl)->finalizeThreadElement(threadClient);
}
