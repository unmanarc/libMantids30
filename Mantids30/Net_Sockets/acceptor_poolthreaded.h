#pragma once

#include "acceptor_callbacks.h"
#include "socket_stream.h"

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Threads/threaded.h>
#include <Mantids30/Threads/threadpool.h>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// TODO: statistics
// TODO: implement max concurrent clients

namespace Mantids30::Network::Sockets::Acceptors {
/**
 * @brief The PoolThreaded class
 */
class PoolThreaded : public Mantids30::Threads::Threaded
{
public:
    /**
       * @brief PoolThreaded Constructor
       * @param acceptorSocket Pre-initialized acceptor socket
       * @param _CallbackTask Callback for succeed inserted task
       * @param _CallbackFailed  Callback when task insertion failed (saturation)
       * @param obj Object to be passed to callbacks
       */
    PoolThreaded();

    // Destructor:
    ~PoolThreaded() override = default;

    /**
       * @brief run Don't call this function, call start(). This is a virtual
       * function for the processor thread.
       */
    void run();
    /**
       * @brief stop Call to stop the acceptor and automatically delete/destroy this
       * class (don't call anything after this).
       */
    void stop();

    /////////////////////////////////////////////////////////////////////////
    // TUNNING:

    class Config
    {
    public:
        /**
         * @brief setConfig Set configuration from a Boost Property Tree.
         * @param pt Boost Property Tree containing configuration.
         */
        void setConfig(const boost::property_tree::ptree &ptree)
        {
            try
            {
                timeoutMS = ptree.get<uint32_t>("TimeoutInMilliseconds", timeoutMS);
                threadsCount = ptree.get<uint32_t>("ThreadsCount", threadsCount);
                taskQueues = ptree.get<uint32_t>("TaskQueues", taskQueues);
                queuesKeyRatio = ptree.get<float>("QueuesKeyRatio", queuesKeyRatio);

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

        /**
     * @brief timeoutMS Timeout in milliseconds to cease trying to insert the
     * task in a queue.
     */
        uint32_t timeoutMS = 5000;

        /**
     * @brief threadsCount Number of threads to be used (must be set before
     * start).
     */
        uint32_t threadsCount = 52;

        /**
     * @brief taskQueues Number of queues to store tasks, each queue handles up
     * to 100 tasks in wait mode.
     */
        uint32_t taskQueues = 36;

        /**
     * @brief queuesKeyRatio Defines how many queues can be used by a specific
     * key. 0.0 means only 1 queue, 1.0 means all queues are available for
     * usage. Default value: 0.5 (half of the queues).
     */
        float queuesKeyRatio = 0.5;

        struct DebugOptions
        {
            std::atomic<bool> enabled{false};
            std::atomic<bool> printPlainText{false};
            std::atomic<bool> printHex{true};
            std::string dir = "/tmp";
        };

        DebugOptions debugOptions;
    };

    Config parameters;
    ////////

    /**
       * @brief addAcceptorSocket Add an acceptor socket for accepting new clients from a distinct source.
       * Multiple acceptor sockets can be added, each will be polled in the run loop.
       * @param value acceptor socket
       */
    void addAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream> &value);
    /**
       * @brief getAcceptorSocketCount Get the number of acceptor sockets currently registered.
       * @return number of acceptor sockets
       */
    size_t getAcceptorSocketCount() const { return m_acceptorSocketList.size(); }

    ThreadPoolCallbacks callbacks;

private:
    struct sAcceptorTaskData
    {
        ~sAcceptorTaskData()
        {
            if (clientSocket)
            {
                clientSocket->shutdownSocket();
            }
        }

        _callbackConnectionRV onConnect = nullptr;
        _callbackConnectionRV onInitFail = nullptr;

        void *contextOnConnect = nullptr;
        void *contextOnInitFail = nullptr;

        std::string key;

        std::shared_ptr<Sockets::Socket_Stream> clientSocket;
        char pad;
    };

    static void runner(void *data);
    static void stopper(void *data);
    static void acceptorTask(const std::shared_ptr<void> & data);

    void init();

    std::vector<std::shared_ptr<Sockets::Socket_Stream>> m_acceptorSocketList;
    std::vector<std::thread> m_acceptorThreads;
    std::atomic<bool> m_running{false};
    std::mutex m_runMutex;
};

} // namespace Mantids30::Network::Sockets::Acceptors
