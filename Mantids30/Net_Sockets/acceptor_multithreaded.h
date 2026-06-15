#pragma once

#include <atomic>
#include <boost/property_tree/ptree.hpp>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <thread>

#include "acceptor_thread.h"
#include "socket_stream.h"

namespace Mantids30::Network::Sockets::Acceptors {

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
     * Add an acceptor socket for accepting new clients from a distinct source.
     * Multiple acceptor sockets can be added, each running in its own thread.
     */
    void addAcceptorSocket(const std::shared_ptr<Socket_Stream> &acceptorSocket);
    /**
     * Get the number of acceptor sockets currently registered.
     */
    size_t getAcceptorSocketCount() const { return m_acceptorSocketList.size(); }
    /**
     * Do accept on the acceptor socket at the given index.
     * @param socketIndex Index of the acceptor socket (default 0 for backward compatibility)
     * @return true if we can still accept a new connection
     */
    bool acceptClient(size_t socketIndex = 0);
    /**
     * Finalize/Catch the client thread element (when it finishes).
     */
    bool finalizeThreadElement(std::shared_ptr<StreamAcceptorThread> x);

    /**
     * @brief The Config class holds configuration parameters for managing concurrency,
     *        connection limits, and timeout settings in a multithreaded environment.
     */
    class Config
    {
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

        /**
         * @brief setConfig Set configuration parameters using a Boost Property Tree.
         * @param ptree The Boost Property Tree containing the configuration parameters.
         */
        void setConfig(const boost::property_tree::ptree &ptree)
        {
            try
            {
                maxConcurrentClients = ptree.get<uint32_t>("MaxConcurrentClients", maxConcurrentClients.load());
                maxWaitMSTime = ptree.get<uint32_t>("MaxWaitTimeInMilliseconds", maxWaitMSTime.load());
                maxConnectionsPerIP = ptree.get<uint32_t>("MaxConcurrentConnectionsPerIP", maxConnectionsPerIP.load());

                debugOptions.enabled = ptree.get<bool>("Debug.Enabled", debugOptions.enabled.load());
                debugOptions.printPlainText = ptree.get<bool>("Debug.PrintPlainText", debugOptions.printPlainText.load());
                debugOptions.printHex = ptree.get<bool>("Debug.PrintHex", debugOptions.printHex.load());
                debugOptions.dir = ptree.get<std::string>("Debug.Dir", debugOptions.dir);
            }
            catch (const std::exception &e)
            {
                // Handle exceptions (e.g., invalid property tree format)
                throw std::runtime_error("Failed to set configuration: " + std::string(e.what()));
            }
        }

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


        struct DebugOptions {
            std::atomic<bool> enabled{false};
            std::atomic<bool> printPlainText{false};
            std::atomic<bool> printHex{true};
            std::string dir = "/tmp";
        };

        DebugOptions debugOptions;



        MultiThreaded *parent = nullptr;
    };

    Config parameters;

private:
    static void thread_streamaccept(const std::shared_ptr<MultiThreaded> &tc, size_t socketIndex);

    bool processClient(std::shared_ptr<Sockets::Socket_Stream> clientSocket, std::shared_ptr<StreamAcceptorThread> clientThread);

    uint32_t incrementIPUsage(const std::string &ipAddr);
    void decrementIPUsage(const std::string &ipAddr);

    bool m_initialized = false;
    bool m_finalized = false;
    std::vector<std::shared_ptr<Sockets::Socket_Stream>> m_acceptorSocketList;
    std::list<std::shared_ptr<StreamAcceptorThread>> m_threadList;
    std::map<std::string, uint32_t> m_connectionsPerIP;

    // thread objects (one per acceptor socket):
    std::vector<std::thread> m_acceptorThreadList;

    //
    std::mutex m_mutexClients;
    std::condition_variable m_condClientsEmpty, m_condClientsNotFull;
};

} // namespace Mantids30::Network::Sockets::Acceptors
