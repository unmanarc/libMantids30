#pragma once

#include "callbacks_socket_tls_client.h"
#include "callbacks_socket_tls_server.h"

namespace Mantids29 { namespace Network { namespace Sockets {

class Socket_TLS_ListennerAndConnector_Base
{
public:
    Socket_TLS_ListennerAndConnector_Base();

    class ServerParameters : public Callbacks_Socket_TLS_Server {
    public:
        ServerParameters(void * obj = nullptr) : Callbacks_Socket_TLS_Server(obj)
        {
            listenPort = 11000;
            listenAddr = "*";
        }

        /**
         * @brief caCertPath CA PEM Certificate path (if empty, will not authenticate the users)
         */
        std::string caCertPath;
        /**
         * @brief listenAddr Listen Address (default *)
         */
        std::string listenAddr;
        /**
         * @brief listenPort TCP Listen Port
         */
        uint16_t listenPort;
        /**
         * @brief keyPath KEY certificate file
         */
        std::string keyPath;
        /**
         * @brief crtPath PEM Certificate file
         */
        std::string crtPath;
    };

    struct IncommingConnectionParams {
        ServerParameters parameters;
        Socket_TLS_ListennerAndConnector_Base * thisObj;
    };

    class ClientParameters : public Callbacks_Socket_TLS_Client {
    public:
        ClientParameters(void * obj = nullptr) : Callbacks_Socket_TLS_Client(obj)
        {
        }

        /**
         * @brief caCertPath
         */
        std::string caCertPath;
        /**
         * @brief expectedCN
         */
        std::string expectedCN;
        /**
         * @brief host
         */
        std::string host;
        /**
         * @brief port
         */
        uint16_t port = 9443;
        /**
         * @brief keyPath
         */
        std::string keyPath;
        /**
         * @brief crtPath
         */
        std::string crtPath;

        bool useSystemX509Certificates = false;
        std::string userCACertificateText;
    };

    /**
     * @brief startListening Start a background process listening... every client will be threaded in a new thread...
     *                       Every client will have a random ID (Session ID)
     * @param parameters server parameters (port/listenaddr/keys/callbacks)
     * @return true if listening, false otherwise.
     */
    bool startListening(const ServerParameters &parameters, void * obj);
    /**
     * @brief startConnecting Start a background process connecting to the specified host...
     *                        The server will be called "SERVER"
     * @param parameters client connection parameters (including callbacks)
     */
    void startConnectionLoop(const ClientParameters &parameters);


    // Callbacks from thread:
    virtual int connectionHandler(Mantids29::Network::Sockets::Socket_TLS * stream, bool remotePeerIsServer, const char * remotePair) = 0;


private:
    static bool incommingConnection(void *obj, Mantids29::Network::Sockets::Socket_Stream_Base * bsocket, const char *, bool secure);

};

}}}


