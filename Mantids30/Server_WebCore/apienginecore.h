#pragma once

#include "apiclienthandler.h"
#include "apiserverconfig.h"
#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Net_Sockets/acceptor_poolthreaded.h>
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <Mantids30/Net_Sockets/socket_stream_dummy.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <memory>

namespace Mantids30::Network::Servers::Web {

class APIEngineCore
{
public:

    struct Callbacks
    {
        /**
         * @brief NotificationCallback A callback function that is invoked when a new client connection is established.
         *
         * This callback allows you to perform actions when a new client successfully connects to the server.
         *
         * @param context A user-defined context that can be passed to the callback function.
         * @param sock The newly established socket connection.
         * @return True to continue processing the connection, false to cancel it.
         */
        struct NotificationCallback
        {
            NotificationCallback()
            {
            }

            /**
             * @brief NotificationCallback Constructor.
             *
             * @param callbackFunction The callback function to be invoked.
             */
            NotificationCallback( bool (*callbackFunction)(void *, std::shared_ptr<Sockets::Socket_Stream>) )
            {
                this->callbackFunction=callbackFunction;
            }

            /**
             * @brief call Invokes the callback function with the provided context and socket.
             *
             * @param context A user-defined context that can be passed to the callback function.
             * @param sock The socket connection to be processed.
             * @return True if the callback returns true, false otherwise.
             */
            bool call(void *context, std::shared_ptr<Sockets::Socket_Stream> sock) const
            {
                if (!callbackFunction)
                    return true;
                return callbackFunction(context, sock);
            }
            /**
             * @brief The callback function to be invoked when a new client connection is established.
             *
             * @param context A user-defined context that can be passed to the callback function.
             * @param sock The newly established socket connection.
             * @return True to continue processing the connection, false to cancel it.
             */
            bool (*callbackFunction)(void *, std::shared_ptr<Sockets::Socket_Stream>) = nullptr;
        };

        NotificationCallback onClientConnected;            ///< Callback invoked when a new client connects.
        NotificationCallback onProtocolInitializationFailure; ///< Callback invoked when protocol initialization fails.
        NotificationCallback onClientAcceptTimeoutOccurred;  ///< Callback invoked when a client connection times out during acceptance.
        NotificationCallback onClientConnectionLimitPerIPReached; ///< Callback invoked when the connection limit per IP address is reached.
    };

    APIEngineCore();

    bool handleVirtualConnection(std::shared_ptr<Network::Sockets::Socket_Stream_Dummy> virtualString);

    /**
     * @brief setAcceptMultiThreaded Configures the server to start in multi-threaded mode where the number of threads grows as connections are received.
     *
     * In this mode, each incoming connection is handled by a new thread up to the specified maximum number of concurrent connections.
     *
     * @param listenerSocket The prepared listener socket (e.g., TCP, TLS) that will be used to accept incoming connections.
     * @param maxConcurrentConnections The maximum number of allowed concurrent connections/threads. Default value is 10000.
     */
    void setAcceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream> &listenerSocket, const uint32_t &maxConcurrentConnections = 10000);

    /**
     * @brief setAcceptPoolThreaded Configures the server to start in pool-threaded mode with a fixed number of pre-started threads.
     *
     * In this mode, incoming connections are queued and handled by a fixed pool of worker threads. Each thread can handle multiple connections up to the specified task queue limit.
     *
     * @param listenerSocket The prepared listener socket (e.g., TCP, TLS) that will be used to accept incoming connections.
     * @param threadCount The initial number of pre-started threads in the pool. Default value is 20.
     * @param taskQueues The maximum number of queued connections per thread. Default value is 100.
     */
    void setAcceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream> &listenerSocket, const uint32_t &threadCount = 20, const uint32_t &taskQueues = 100);

    /**
     * @brief startInBackground Starts the server in the background.
     *
     * This method will initiate the server's operation based on the configuration set by either `setAcceptMultiThreaded` or `setAcceptPoolThreaded`.
     */
    void startInBackground();

    // Seteables (before starting the acceptor, non-thread safe):
    Callbacks callbacks;                    ///< The callbacks object used by the web server.
    APIServerConfig config;             ///< The api server configuration parameters

    std::shared_ptr<Sockets::Socket_Stream> getListenerSocket() const;
    /**
     * @brief setWebsocketEndpoints Set websocket endpoints and overwrite configuration parameters.
     * @param newWebsocketEndpoints
     */
    void setWebsocketEndpoints(const std::shared_ptr<API::WebSocket::Endpoints> &newWebsocketEndpoints);

protected:
    virtual std::shared_ptr<APIClientHandler> createNewAPIClientHandler(APIEngineCore * webServer, std::shared_ptr<Sockets::Socket_Stream> s ) { return nullptr; }

    /**
     * @brief checkEngineStatus Check if the engine is properly configured to be started or not.
     *                          if is not properly configured, you should destroy everything because is a programming error.
     *                          by example: not initializing some required variable should trigger an uncaught exeption.
     */
    virtual void checkEngineStatus() {}

    /**
     * @brief m_websocketEndpoints WebSocket endpoints shared pointer
     */
    std::shared_ptr<API::WebSocket::Endpoints> m_websocketEndpoints;
private:
    enum class AcceptorType {
        NONE,
        POOL_THREADED,
        MULTI_THREADED,
    };

    AcceptorType m_acceptorType = AcceptorType::NONE;
    std::shared_ptr<Sockets::Socket_Stream> m_listenerSocket;

    std::shared_ptr<Network::Sockets::Acceptors::MultiThreaded> m_multiThreadedAcceptor;
    std::shared_ptr<Network::Sockets::Acceptors::PoolThreaded> m_poolThreadedAcceptor;

    /**
     * callback when connection is fully established (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool handleConnect(void *, std::shared_ptr<Sockets::Socket_Stream>);
    /**
     * callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool handleInitFailed(void *, std::shared_ptr<Sockets::Socket_Stream>);
    /**
     * callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    static void handleTimeOut(void *, std::shared_ptr<Sockets::Socket_Stream>);

    /**
     * @brief _onConnectionLimit
     */
    static void handleConnectionLimit(void *, std::shared_ptr<Sockets::Socket_Stream>);


};

}
