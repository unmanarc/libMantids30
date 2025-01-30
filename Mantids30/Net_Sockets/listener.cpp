#include "listener.h"
#include "socket_stream_base.h"

#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <memory>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30;
using namespace std;

Listener::Listener() {}

bool Listener::incomingConnection(std::shared_ptr<void> context,
                                  std::shared_ptr<Sockets::Socket_Stream_Base> socketStream,
                                  const char *clientIp,
                                  bool isSecure)
{
    ThreadParameters *threadParams = (ThreadParameters *)(context.get());

    CALLBACK(threadParams->parameters.tcpCallbacks.onClientConnected)(threadParams->parameters.context, socketStream);

    auto connectionStatus = threadParams->thisObj->handleClientConnection(socketStream, clientIp);

    CALLBACK(threadParams->parameters.tcpCallbacks.onClientDisconnected)(threadParams->parameters.context, socketStream, connectionStatus);

    return true;
}


bool Listener::startListening(const Config &parameters )
{
    // TODO: this is repeated code... we should pass this to socket_tls? and personalize the socket options...
    shared_ptr<Acceptors::MultiThreaded> multiThreadedAcceptor = make_shared<Acceptors::MultiThreaded>();

    shared_ptr<Socket_Stream_Base> listenerSocket = parameters.useTLS ? std::make_shared<Socket_TLS>() : std::make_shared<Socket_TCP>();

    bool cont = true;

    if ( parameters.useTLS )
    {
        auto tlsSocket = std::dynamic_pointer_cast<Socket_TLS>(listenerSocket);
        if (tlsSocket)
        {
            if (!parameters.tlsCACertificatePath.empty() && !tlsSocket->m_keys.loadCAFromPEMFile(parameters.tlsCACertificatePath))
            {
                // Error loading Optional CA PEM file...
                CALLBACK(parameters.tlsCallbacks.onInvalidCACertificate)(parameters.context, tlsSocket, parameters.tlsCACertificatePath);
                cont = false;
            }
            if (!parameters.tlsPrivateKeyPath.empty() && !parameters.tlsCertificatePath.empty())
            {
                if (!tlsSocket->m_keys.loadPrivateKeyFromPEMFile(parameters.tlsPrivateKeyPath.c_str()))
                {
                    CALLBACK(parameters.tlsCallbacks.onInvalidPrivateKey)(parameters.context, tlsSocket, parameters.tlsPrivateKeyPath);
                    cont = false;
                }
                if (!tlsSocket->m_keys.loadPublicKeyFromPEMFile(parameters.tlsCertificatePath.c_str()))
                {
                    CALLBACK(parameters.tlsCallbacks.onInvalidClientCertificate)(parameters.context, tlsSocket, parameters.tlsCertificatePath);
                    cont = false;
                }
            }
        }
    }

    listenerSocket->setUseIPv6( parameters.useIPv6 );

    if (!cont || !listenerSocket->listenOn(parameters.listenPort, parameters.listenAddr.c_str()))
    {
        CALLBACK(parameters.tcpCallbacks.onListeningFailed)(parameters.context, listenerSocket);
        return false;
    }

    CALLBACK(parameters.tcpCallbacks.onListeningSuccess)(parameters.context, listenerSocket);

    std::shared_ptr<ThreadParameters> threadParams = std::make_shared<ThreadParameters>();
    threadParams->thisObj = this;
    threadParams->parameters = parameters;

    // STREAM MANAGER:
    multiThreadedAcceptor->setAcceptorSocket(listenerSocket);

    multiThreadedAcceptor->setMaxConcurrentClients(parameters.maxConcurrentClients);
    multiThreadedAcceptor->setMaxConnectionsPerIP(parameters.maxConnectionsPerIP);
    //multiThreadedAcceptor->setMaxWaitMSTime();

    // Set callbacks:
    multiThreadedAcceptor->setCallbackOnConnect(&incomingConnection, threadParams);
    multiThreadedAcceptor->setCallbackOnMaxConnectionsPerIP(parameters.tcpCallbacks.onClientConnectionLimitPerIPReached, parameters.context);
    multiThreadedAcceptor->setCallbackOnInitFail(parameters.tlsCallbacks.onClientAuthenticationError, parameters.context);
    multiThreadedAcceptor->setCallbackOnTimedOut(parameters.tcpCallbacks.onClientAcceptTimeoutOccurred, parameters.context);

    multiThreadedAcceptor->startThreaded(multiThreadedAcceptor);

    return true;
}
