#pragma once

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <condition_variable>

#include "acceptor_thread.h"
#include "socket_stream.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace Acceptors {

/**
 * @brief The MultiThreadedAcceptor class Accept streams on thread from a listening socket.
 */
class MultiThreaded : public std::enable_shared_from_this<MultiThreaded>
{
public:

    /**
     * Constructor
     */
    MultiThreaded();
    /**
     * @brief MultiThreaded Integrated constructor with all the initial parameters (after that, you are safe to run startThreaded or startBlocking)
     * @param acceptorSocket acceptor socket
     * @param _onConnect callback function on connect (mandatory: this will handle the connection itself)
     * @param context object passed to all callbacks
     * @param _onInitFailed callback function on failed initialization (default nullptr -> none)
     * @param _onTimeOut callback function on time out (default nullptr -> none)
     * @param _onClientConnectionLimitPerIPReached callback function when an ip reached the max number of connections (default nullptr -> none)
     */
    MultiThreaded(const std::shared_ptr<Socket_Stream> &acceptorSocket,
                    _callbackConnectionRB _onConnect,
                    void * context=nullptr,
                    _callbackConnectionRB _onInitFailed=nullptr,
                    _callbackConnectionRV _onTimeOut=nullptr,
                    _callbackConnectionLimit _onClientConnectionLimitPerIPReached=nullptr
                    );
    /**
     * Destructor
     * WARN: when you finalize this class, the listening socket is closed. please open another one (don't reuse it)
     */
    ~MultiThreaded();

    MultiThreadCallbacks callbacks;

    /**
     * @brief startThreaded Start accepting connections in a new thread (will wait for finalization in destructor)
     */
    void startInBackground();
    /**
     * @brief startBlocking Start Accepting Connections in your own thread.
     * @return
     */
    bool startBlocking();
    /**
     * @brief stop Stop Acceptor
     */
    void stop();

    /**
     * Set the socket that will be used to accept new clients.
     * WARNING: acceptorSocket will be deleted when this class finishes.
     */
    void setAcceptorSocket(const std::shared_ptr<Socket_Stream> &acceptorSocket);
    /**
     * Do accept on the acceptor socket.
     * @return true if we can still accept a new connection
     */
    bool acceptClient();
    /**
     * Finalize/Catch the client thread element (when it finishes).
     */
    bool finalizeThreadElement(std::shared_ptr<StreamAcceptorThread> x);

    /**
     * @brief The Config class holds configuration parameters for managing concurrency,
     *        connection limits, and timeout settings in a multithreaded environment.
     */
    class Config {
    public:
        friend class MultiThreaded;
        Config() = default;
        /**
         * @brief getMaxConcurrentClients Get maximum number of concurrent client threads are accepted
         * @return maximum current clients accepted
         */
        uint32_t getMaxConcurrentClients();
        /**
         * @brief setMaxConcurrentClients Set maximum number of client threads accepted at the same time. Beware that too many concurrent threads could lead to an unhandled exception an program fault (check your ulimits).
         * @param value maximum current clients accepted
         */
        void setMaxConcurrentClients(const uint32_t &value);
        /**
         * @brief getMaxWaitMSTime Get maximum time to wait if maximum concurrent limit reached
         * @return time in milliseconds
         */
        uint32_t getMaxWaitMSTime();
        /**
         * @brief setMaxWaitMSTime Set maximum time to wait if maximum concurrent limit reached
         * @param value time in milliseconds
         */
        void setMaxWaitMSTime(const uint32_t &value);
        /**
         * @brief getMaxConnectionsPerIP Get maximum concurrent connections per client IP
         * @return maximum number of connections allowed
         */
        uint32_t getMaxConnectionsPerIP();
        /**
         * @brief setMaxConnectionsPerIP Set maximum concurrent connections per client IP
         * @param value maximum number of connections allowed
         */
        void setMaxConnectionsPerIP(const uint32_t &value);

        void setParent(MultiThreaded *newParent);

    private:
        /**
         * @brief maxConcurrentClients Defines the maximum number of client threads that can be handled concurrently.
         *        If the number of active clients exceeds this limit, new connections may be rejected or queued.
         *        Be cautious when increasing this value, as too many concurrent threads could lead to system instability
         *        or crashes due to resource exhaustion (check your ulimits).
         */
        std::atomic<uint32_t> maxConcurrentClients{100};

        /**
         * @brief maxWaitMSTime Defines the maximum time (in milliseconds) a new client connection will wait
         *        if the maximum concurrent client limit is reached.
         *        If the wait time is exceeded, the connection attempt will be aborted.
         */
        std::atomic<uint32_t> maxWaitMSTime{5000};

        /**
         * @brief maxConnectionsPerIP Defines the maximum number of concurrent connections allowed per unique client IP.
         *        This helps prevent a single IP from monopolizing server resources and ensures fair distribution.
         */
        std::atomic<uint32_t> maxConnectionsPerIP{10};

        MultiThreaded * parent = nullptr;
    };

    Config parameters;

private:
    static void thread_streamaccept(const std::shared_ptr<MultiThreaded> &tc);

    bool processClient(std::shared_ptr<Sockets::Socket_Stream> clientSocket, std::shared_ptr<StreamAcceptorThread> clientThread);

    uint32_t incrementIPUsage(const std::string & ipAddr);
    void decrementIPUsage(const std::string & ipAddr);


    bool m_initialized=false;
    bool m_finalized=false;
    std::shared_ptr<Sockets::Socket_Stream> m_acceptorSocket;
    std::list<std::shared_ptr<StreamAcceptorThread>> m_threadList;
    std::map<std::string, uint32_t> m_connectionsPerIP;

    // thread objects:
    std::thread m_acceptorThread;

    //
    std::mutex m_mutexClients;
    std::condition_variable m_condClientsEmpty, m_condClientsNotFull;
};


}}}}
