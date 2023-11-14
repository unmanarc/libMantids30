#include "socket_tls_listennerandconnector_base.h"

#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30;
using namespace std;

Socket_TLS_ListennerAndConnector_Base::Socket_TLS_ListennerAndConnector_Base()
{
    m_stopReconnecting = false;
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
        CALLBACK(parameters.onTLSKeyInvalidCA)(parameters.obj, tlsSocket.get(), parameters.caCertPath);
        cont = false;
    }
    if (!parameters.keyPath.empty() && !parameters.crtPath.empty())
    {
        if (!tlsSocket->m_keys.loadPrivateKeyFromPEMFile(parameters.keyPath.c_str()))
        {
            CALLBACK(parameters.onTLSKeyInvalidPrivateKey)(parameters.obj, tlsSocket.get(), parameters.keyPath);
            cont = false;
        }
        if (!tlsSocket->m_keys.loadPublicKeyFromPEMFile(parameters.crtPath.c_str()))
        {
            CALLBACK(parameters.onTLSKeyInvalidCertificate)(parameters.obj, tlsSocket.get(), parameters.crtPath);
            cont = false;
        }
    }

    if (!cont || !tlsSocket->listenOn(parameters.listenPort, parameters.listenAddr.c_str(), true))
    {
        CALLBACK(parameters.onTLSListeningFailed)(parameters.obj, tlsSocket.get());
        return false;
    }

    CALLBACK(parameters.onTLSListeningSuccess)(parameters.obj, tlsSocket.get());

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

bool Socket_TLS_ListennerAndConnector_Base::incommingConnection(void *obj,
                                                                Mantids30::Network::Sockets::Socket_Stream_Base *bsocket,
                                                                const char *ip,
                                                                bool secure)
{
    Socket_TLS *tlsSocket = (Socket_TLS *) bsocket;
    auto pararms = (IncommingConnectionParams *) obj;

    CALLBACK(pararms->parameters.onTLSClientConnected)(pararms->parameters.obj, tlsSocket, tlsSocket->getTLSPeerCN());

    auto i = pararms->thisObj->handleConnection(tlsSocket, false, ip);

    CALLBACK(pararms->parameters.onTLSClientDisconnected)(pararms->parameters.obj, tlsSocket, tlsSocket->getTLSPeerCN(), i);

    delete pararms;
    return true;
}


void connectionLoopThread(Socket_TLS_ListennerAndConnector_Base *parent,
                          const shared_ptr<Socket_TLS_ListennerAndConnector_Base::ClientParameters> &parameters)
{
    bool cont = true;

    int retries=0;

    for (;cont;)
    {
        Socket_TLS tlsSocket;

        if (retries == parameters->maxRetries)
        {
            CALLBACK(parameters->onTLSConnectionRetriesReached)(parameters->obj, &tlsSocket, parameters->host.c_str(), parameters->port);
            break;
        }

        CALLBACK(parameters->onTLSConnectionStart)(parameters->obj, &tlsSocket, parameters->host.c_str(), parameters->port);

        // CA Cert Path Defined.
        if (!parameters->caCertPath.empty())
        {
            if (!tlsSocket.m_keys.loadCAFromPEMFile(parameters->caCertPath))
            {
                // Error loading Mandatory CA PEM file...
                CALLBACK(parameters->onTLSKeyInvalidCA)(parameters->obj, &tlsSocket, parameters->caCertPath);
                cont = false;
            }
        }
        else if (parameters->useSystemX509Certificates)
        {
            tlsSocket.m_keys.setUseSystemCertificates(true);
        }
        else if (!parameters->userCACertificateText.empty())
        {
            if (!tlsSocket.m_keys.loadCAFromPEMMemory(parameters->userCACertificateText.c_str()))
            {
                CALLBACK(parameters->onTLSKeyInvalidCA)(parameters->obj, &tlsSocket, parameters->caCertPath);
                cont = false;
            }
        }

        if (!parameters->keyPath.empty() && !parameters->crtPath.empty())
        {
            if (!tlsSocket.m_keys.loadPrivateKeyFromPEMFile(parameters->keyPath.c_str()))
            {
                CALLBACK(parameters->onTLSKeyInvalidPrivateKey)(parameters->obj, &tlsSocket, parameters->keyPath);
                cont = false;
            }
            if (!tlsSocket.m_keys.loadPublicKeyFromPEMFile(parameters->crtPath.c_str()))
            {
                CALLBACK(parameters->onTLSKeyInvalidCertificate)(parameters->obj, &tlsSocket, parameters->crtPath);
                cont = false;
            }
        }

        if (cont)
        {
            // Use connection loop...
            if (!tlsSocket.connectTo(parameters->host.c_str(), parameters->port))
            {
                retries++;
                // Error connecting...
                if (!(CALLBACK_B(parameters->onTLSConnectionFailed, true)(parameters->obj, &tlsSocket, parameters->host.c_str(), parameters->port)))
                {
                    cont = false;
                }
            }
            else
            {
                retries=0;

                // Connected OK...
                CALLBACK(parameters->onTLSConnectionSuccess)(parameters->obj, &tlsSocket);

                char remotePair[65];
                tlsSocket.getRemotePair(remotePair);
                auto i = parent->handleConnection(&tlsSocket, true, remotePair);

                CALLBACK(parameters->onTLSDisconnected)(parameters->obj, &tlsSocket, parameters->host.c_str(), parameters->port, i);

                if (parent->m_stopReconnecting)
                {
                    cont = false;
                    parent->m_stopReconnecting = true;
                }
            }
        }
    }
}

void Socket_TLS_ListennerAndConnector_Base::startConnectionLoopThread(const ClientParameters &parameters)
{
    thread t = thread(connectionLoopThread, this, make_shared<ClientParameters>(parameters));
    t.detach();
}
