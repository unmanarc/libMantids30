#include "listener.h"
#include "socket_stream.h"

#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <memory>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30;
using namespace std;

Listener::Listener() {}

bool Listener::incomingConnection(void * context,
                                  std::shared_ptr<Sockets::Socket_Stream> socketStream)
{
    Listener *threadParams = (Listener *)(context);

    CALLBACK(threadParams->tcpCallbacks.onClientConnected)(threadParams->listenerContext, socketStream);

    auto connectionStatus = threadParams->handleClientConnection(socketStream);

    CALLBACK(threadParams->tcpCallbacks.onClientDisconnected)(threadParams->listenerContext, socketStream, connectionStatus);

    return true;
}


bool Listener::startListeningInBackground( const Config &parameters )
{
    // TODO: this is repeated code... we should pass this to socket_tls? and personalize the socket options...
    shared_ptr<Acceptors::MultiThreaded> multiThreadedAcceptor = make_shared<Acceptors::MultiThreaded>();

    shared_ptr<Socket_Stream> listenerSocket = parameters.useTLS ? std::make_shared<Socket_TLS>() : std::make_shared<Socket_TCP>();

    bool cont = true;

    if ( parameters.useTLS )
    {
        auto tlsSocket = std::dynamic_pointer_cast<Socket_TLS>(listenerSocket);
        if (tlsSocket)
        {
            if (!parameters.tlsCACertificatePath.empty() && !tlsSocket->tlsKeys.loadCAFromPEMFile(parameters.tlsCACertificatePath))
            {
                // Error loading Optional CA PEM file...
                CALLBACK(tlsCallbacks.onInvalidCACertificate)(listenerContext, tlsSocket, parameters.tlsCACertificatePath);
                cont = false;
            }
            if (!parameters.tlsPrivateKeyPath.empty() && !parameters.tlsCertificatePath.empty())
            {
                if (!tlsSocket->tlsKeys.loadPrivateKeyFromPEMFile(parameters.tlsPrivateKeyPath.c_str()))
                {
                    CALLBACK(tlsCallbacks.onInvalidPrivateKey)(listenerContext, tlsSocket, parameters.tlsPrivateKeyPath);
                    cont = false;
                }
                if (!tlsSocket->tlsKeys.loadPublicKeyFromPEMFile(parameters.tlsCertificatePath.c_str()))
                {
                    CALLBACK(tlsCallbacks.onInvalidClientCertificate)(listenerContext, tlsSocket, parameters.tlsCertificatePath);
                    cont = false;
                }
            }
        }
    }

    listenerSocket->setUseIPv6( parameters.useIPv6 );

    if (!cont || !listenerSocket->listenOn(parameters.listenPort, parameters.listenAddr.c_str()))
    {
        CALLBACK(tcpCallbacks.onListeningFailed)(listenerContext, listenerSocket);
        return false;
    }

    CALLBACK(tcpCallbacks.onListeningSuccess)(listenerContext, listenerSocket);

    // STREAM MANAGER:
    multiThreadedAcceptor->setAcceptorSocket(listenerSocket);

    multiThreadedAcceptor->parameters.setMaxConcurrentClients(parameters.maxConcurrentClients);
    multiThreadedAcceptor->parameters.setMaxConnectionsPerIP(parameters.maxConnectionsPerIP);
    //multiThreadedAcceptor->setMaxWaitMSTime();

    // Set callbacks:
    multiThreadedAcceptor->callbacks.contextOnConnect = this;
    multiThreadedAcceptor->callbacks.contextOnInitFail = listenerContext;
    multiThreadedAcceptor->callbacks.contextonClientConnectionLimitPerIPReached = listenerContext;
    multiThreadedAcceptor->callbacks.contextOnTimedOut = listenerContext;

    multiThreadedAcceptor->callbacks.onClientConnected = &incomingConnection;
    multiThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = tcpCallbacks.onClientAcceptTimeoutOccurred;
    multiThreadedAcceptor->callbacks.onClientConnectionLimitPerIPReached = tcpCallbacks.onClientConnectionLimitPerIPReached;
    multiThreadedAcceptor->callbacks.onProtocolInitializationFailure = tlsCallbacks.onProtocolInitializationFailure;

/*
    multiThreadedAcceptor->setCallbackOnConnect(&incomingConnection, threadParams);
    multiThreadedAcceptor->setCallbackonClientConnectionLimitPerIPReached(parameters.tcpCallbacks.onClientConnectionLimitPerIPReached, listenerContext);
    multiThreadedAcceptor->setCallbackOnInitFail(parameters.tlsCallbacks.onProtocolInitializationFailure, listenerContext);
    multiThreadedAcceptor->setCallbackOnTimedOut(, listenerContext);*/

    multiThreadedAcceptor->startInBackground();

    return true;
}
