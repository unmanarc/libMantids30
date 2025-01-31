#include "connector.h"
#include "Mantids30/Helpers/callbacks.h"
#include "socket_stream_base.h"
#include <memory>
#include <netinet/in.h>

using namespace Mantids30::Network::Sockets;
using namespace Mantids30;
using namespace std;


void connectionLoopThread(Connector *parent,
                          const shared_ptr<Connector::Config> &config)
{
    bool cont = true;

    parent->m_stopReconnecting = false;

    int retries=0;

    for (;cont;)
    {
        std::shared_ptr<Socket_Stream_Base> clientSocket;

        if (config->useTLS)
        {
            clientSocket = make_shared<Socket_TLS>();
        }
        else
        {
            clientSocket = make_shared<Socket_TCP>();
        }

        if (retries == config->maxRetries || parent->m_stopReconnecting)
        {
            CALLBACK(config->tcpCallbacks.onMaxRetryLimitReached)(config->context, clientSocket, config->host.c_str(), config->port);
            break;
        }

        CALLBACK(config->tcpCallbacks.onPreConnectionAttempt)(config->context, clientSocket, config->host.c_str(), config->port);

        if (config->useTLS)
        {
            auto tlsSocket = std::dynamic_pointer_cast<Socket_TLS>(clientSocket);
            if (tlsSocket)
            {
                // CA Cert Path Defined.
                if (!config->tlsCACertificatePath.empty())
                {
                    if (!tlsSocket->m_keys.loadCAFromPEMFile(config->tlsCACertificatePath))
                    {
                        // Error loading Mandatory CA PEM file...
                        CALLBACK(config->tlsCallbacks.onInvalidCACertificate)(config->context, tlsSocket, config->tlsCACertificatePath);
                        cont = false;
                    }
                }
                else if (config->tlsUseSystemX509Certificates)
                {
                    tlsSocket->m_keys.setUseSystemCertificates(true);
                }
                else if (!config->tlsCustomCACertificateText.empty())
                {
                    if (!tlsSocket->m_keys.loadCAFromPEMMemory(config->tlsCustomCACertificateText.c_str()))
                    {
                        CALLBACK(config->tlsCallbacks.onInvalidCACertificate)(config->context, tlsSocket, config->tlsCACertificatePath);
                        cont = false;
                    }
                }
                if (!config->tlsPrivateKeyPath.empty() && !config->tlsCertificatePath.empty())
                {
                    if (!tlsSocket->m_keys.loadPrivateKeyFromPEMFile(config->tlsPrivateKeyPath.c_str()))
                    {
                        CALLBACK(config->tlsCallbacks.onInvalidPrivateKey)(config->context, tlsSocket, config->tlsPrivateKeyPath);
                        cont = false;
                    }
                    if (!tlsSocket->m_keys.loadPublicKeyFromPEMFile(config->tlsCertificatePath.c_str()))
                    {
                        CALLBACK(config->tlsCallbacks.onInvalidClientCertificate)(config->context, tlsSocket, config->tlsCertificatePath);
                        cont = false;
                    }
                }
            }
        }

        if (cont)
        {
            // Use connection loop...
            if (!clientSocket->connectTo(config->host.c_str(), config->port))
            {
                retries++;
                // Error connecting...
                if (!(CALLBACK_B(config->tcpCallbacks.onConnectionFailure, true)(config->context, clientSocket, config->host.c_str(), config->port)))
                {
                    cont = false;
                }
            }
            else
            {
                //retries=0;

                // Connected OK...
                CALLBACK(config->tcpCallbacks.onConnectionEstablished)(config->context, clientSocket);

                char remotePair[INET6_ADDRSTRLEN+2];
                clientSocket->getRemotePair(remotePair);
                int i = parent->handleServerConnection(clientSocket, remotePair);

                CALLBACK(config->tcpCallbacks.onConnectionTerminated)(config->context, clientSocket, config->host.c_str(), config->port, i);
                cont = false;

                /**
                 *  HINT:
                 *
                 *  If you want to persist the connection after this, call another startConnectionLoopThread during the onConnectionTerminated call...
                 */
            }
        }
    }
}



std::thread Mantids30::Network::Sockets::Connector::startConnectionLoopThread(const Config &parameters)
{
    thread t = thread(connectionLoopThread, this, make_shared<Config>(parameters));
    return t;
}
