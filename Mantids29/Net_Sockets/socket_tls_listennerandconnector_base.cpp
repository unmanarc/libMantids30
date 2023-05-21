#include "socket_tls_listennerandconnector_base.h"

#include <Mantids29/Helpers/callbacks.h>
#include <Mantids29/Net_Sockets/acceptor_multithreaded.h>

using namespace Mantids29::Network::Sockets;
using namespace Mantids29;
using namespace std;

Socket_TLS_ListennerAndConnector_Base::Socket_TLS_ListennerAndConnector_Base()
{

}

bool Socket_TLS_ListennerAndConnector_Base::startListening(const ServerParameters &parameters, void *obj)
{
    // TODO: this is repeated code... we should pass this to socket_tls? and personalize the socket options...
    shared_ptr<Acceptors::MultiThreaded> multiThreadedAcceptor = make_shared<Acceptors::MultiThreaded>();
    shared_ptr<Socket_TLS> tlsSocket = std::make_shared<Socket_TLS>();

    bool cont = true;
    
    if (!parameters.caCertPath.empty() && !tlsSocket->m_keys.loadCAFromPEMFile(parameters.caCertPath))
    {
        // Error loading Optional CA PEM file...
        CALLBACK(parameters.onTLSKeyInvalidCA)(parameters.obj,tlsSocket.get(),parameters.caCertPath);
        cont = false;
    }
    if (!parameters.keyPath.empty() && !parameters.crtPath.empty())
    {
        if (!tlsSocket->m_keys.loadPrivateKeyFromPEMFile(parameters.keyPath.c_str()))
        {
            CALLBACK(parameters.onTLSKeyInvalidPrivateKey)(parameters.obj,tlsSocket.get(),parameters.keyPath);
            cont = false;
        }
        if (!tlsSocket->m_keys.loadPublicKeyFromPEMFile(parameters.crtPath.c_str()))
        {
            CALLBACK(parameters.onTLSKeyInvalidCertificate)(parameters.obj,tlsSocket.get(),parameters.crtPath);
            cont = false;
        }
    }

    if (!cont || !tlsSocket->listenOn(parameters.listenPort,parameters.listenAddr.c_str(),true))
    {
        CALLBACK(parameters.onTLSListeningFailed)(parameters.obj,tlsSocket.get());
        return false;
    }

    CALLBACK(parameters.onTLSListeningSuccess)(parameters.obj,tlsSocket.get());

    auto params = new IncommingConnectionParams;
    params->thisObj = this;
    params->parameters = parameters;

    // STREAM MANAGER:
    multiThreadedAcceptor->setAcceptorSocket(tlsSocket);
    multiThreadedAcceptor->setCallbackOnConnect(&incommingConnection, obj);
    multiThreadedAcceptor->setCallbackOnMaxConnectionsPerIP(parameters.onTLSClientConnectionLimitPerIPReached, parameters.obj);
    multiThreadedAcceptor->setCallbackOnInitFail(parameters.onTLSClientAuthenticationError, parameters.obj);
    multiThreadedAcceptor->setCallbackOnTimedOut(parameters.onTLSClientAcceptTimeoutOccurred, parameters.obj);
    multiThreadedAcceptor->startThreaded(multiThreadedAcceptor);

    return true;
}

bool Socket_TLS_ListennerAndConnector_Base::incommingConnection(void *obj, Mantids29::Network::Sockets::Socket_Stream_Base *bsocket, const char *ip, bool secure)
{
    Socket_TLS * tlsSocket = (Socket_TLS *)bsocket;
    auto pararms = (IncommingConnectionParams *)obj;

    CALLBACK(pararms->parameters.onTLSClientConnected)(pararms->parameters.obj,tlsSocket,tlsSocket->getTLSPeerCN());

    auto i = pararms->thisObj->connectionHandler( tlsSocket, false, ip );

    CALLBACK(pararms->parameters.onTLSClientDisconnected)(pararms->parameters.obj,tlsSocket,tlsSocket->getTLSPeerCN(),i);

    delete pararms;
    return true;
}

void connectionLoopThread(Socket_TLS_ListennerAndConnector_Base * parent, const shared_ptr<Socket_TLS_ListennerAndConnector_Base::ClientParameters> & parameters)
{
    for (;;)
    {
        Socket_TLS tlsSocket;

        bool cont = true;
        
        if (!tlsSocket.m_keys.loadCAFromPEMFile(parameters->caCertPath))
        {
            // Error loading Mandatory CA PEM file...
            CALLBACK(parameters->onTLSKeyInvalidCA)(parameters->obj,&tlsSocket,parameters->caCertPath);
            cont = false;
        }
        if (!parameters->keyPath.empty() && !parameters->crtPath.empty())
        {
            if (!tlsSocket.m_keys.loadPrivateKeyFromPEMFile(parameters->keyPath.c_str()))
            {
                CALLBACK(parameters->onTLSKeyInvalidPrivateKey)(parameters->obj,&tlsSocket,parameters->keyPath);
                cont = false;
            }
            if (!tlsSocket.m_keys.loadPublicKeyFromPEMFile(parameters->crtPath.c_str()))
            {
                CALLBACK(parameters->onTLSKeyInvalidCertificate)(parameters->obj,&tlsSocket,parameters->crtPath);
                cont = false;
            }
        }

        if (cont)
        {
            // Use connection loop...
            if (!tlsSocket.connectTo(parameters->host.c_str(),parameters->port))
            {
                // Error connecting...
                CALLBACK(parameters->onTLSConnectionFailed)(parameters->obj, &tlsSocket, parameters->host.c_str(),parameters->port );
            }
            else
            {
                // Connected OK...
                CALLBACK(parameters->onTLSConnectionSuccess)(parameters->obj, &tlsSocket);

                char remotePair[65];
                tlsSocket.getRemotePair(remotePair);
                auto i = parent->connectionHandler( &tlsSocket, true, remotePair );

                CALLBACK(parameters->onTLSDisconnected)(parameters->obj, &tlsSocket, parameters->host.c_str(),parameters->port, i);
            }
        }
    }
}

void Socket_TLS_ListennerAndConnector_Base::startConnectionLoop(const ClientParameters &parameters)
{
    thread t = thread(connectionLoopThread, this, make_shared<ClientParameters>(parameters));
    t.detach();
}
