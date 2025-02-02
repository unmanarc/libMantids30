#pragma once

#include "callbacks_socket_tcp_client.h"
#include "callbacks_socket_tls.h"
#include "socket_stream_base.h"
#include <thread>

namespace Mantids30 { namespace Network { namespace Sockets {

class Connector
{
public:
    Connector() { m_stopReconnecting = false; }


    class Config
    {
    public:
        Config(void *context = nullptr)
            : context(context)
        {}

        // TLS:
        /**
         * @brief Path to the Certificate Authority (CA) PEM file for validating server certificates.
         *        If empty, no server certificate validation will be performed.
         */
        std::string tlsCACertificatePath;

        /**
         * @brief Expected Common Name (CN) in the server's TLS certificate.
         *        Used to validate the identity of the server during the handshake.
         */
        std::string tlsExpectedCommonName;

        /**
         * @brief Path to the private key file (PEM format) for the client's TLS certificate.
         *        Required if mutual TLS authentication is used.
         */
        std::string tlsPrivateKeyPath;

        /**
         * @brief Path to the client's TLS certificate file (PEM format).
         *        Required if mutual TLS authentication is used.
         */
        std::string tlsCertificatePath;

        /**
         * @brief If true, the system's default X.509 certificates will be used for server certificate validation.
         */
        bool tlsUseSystemX509Certificates = false;

        /**
         * @brief Custom CA certificate text (in PEM format) provided directly as a string.
         *        This will be used instead of a CA certificate file if provided.
         */
        std::string tlsCustomCACertificateText;

        bool useTLS = true;

        // TCP:
        /**
         * @brief host
         */
        std::string host;
        /**
         * @brief port
         */
        uint16_t port = 9443;

        uint32_t maxRetries = 5;

        // CAllbacks:
        Callbacks_Socket_TLS tlsCallbacks;
        Callbacks_Socket_TCP_Client tcpCallbacks;

        void *context;
    };


    /**
     * @brief startConnecting Start a background process connecting to the specified host...
     *                        The server will be called "SERVER"
     * @param parameters client connection parameters (including callbacks)
     */
    std::thread startConnectionLoopThread(const Config &parameters);

    // Callbacks from thread:
    virtual int handleServerConnection(std::shared_ptr<Sockets::Socket_Stream_Base> sock) = 0;

    std::atomic_bool m_stopReconnecting;


private:/*
    struct ThreadParameters {
        Config parameters;
        Connector * thisObj;
    };
*/
};

}}}
