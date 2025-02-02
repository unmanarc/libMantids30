#pragma once

#include "socket_stream_base.h"
#include "acceptor_callbacks.h"

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Threads/threaded.h>
#include <Mantids30/Threads/threadpool.h>
#include <memory>
#include <mutex>

// TODO: statistics

namespace Mantids30 { namespace Network { namespace Sockets { namespace Acceptors {
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
    /**
     * @brief PoolThreaded Integrated constructor with all the initial parameters (after that, you are safe to run startThreaded or startBlocking)
     * @param acceptorSocket acceptor socket
     * @param _onConnect callback function on connect (mandatory: this will handle the connection itself)
     * @param context object passed to all callbacks
     * @param _onInitFailed callback function on failed initialization (default nullptr -> none)
     * @param _onTimeOut callback function on time out (default nullptr -> none)
     * @param _onClientConnectionLimitPerIPReached callback function when an ip reached the max number of connections (default nullptr -> none)
     */
    PoolThreaded(const std::shared_ptr<Sockets::Socket_Stream_Base> &acceptorSocket, _callbackConnectionRB _onConnect, void * context = nullptr, _callbackConnectionRB _onInitFailed = nullptr, _callbackConnectionRV _onTimeOut = nullptr);

    // Destructor:
    ~PoolThreaded() override;

    /**
     * @brief run Don't call this function, call start(). This is a virtual function for the processor thread.
     */
    void run();
    /**
     * @brief stop Call to stop the acceptor and automatically delete/destroy this class (don't call anything after this).
     */
    void stop();

    /////////////////////////////////////////////////////////////////////////
    // TUNNING:

    class Config {
    public:
        /**
         * @brief timeoutMS Timeout in milliseconds to cease trying to insert the task in a queue.
         */
        uint32_t timeoutMS = 5000;

        /**
         * @brief threadsCount Number of threads to be used (must be set before start).
         */
        uint32_t threadsCount = 52;

        /**
         * @brief taskQueues Number of queues to store tasks, each queue handles up to 100 tasks in wait mode.
         */
        uint32_t taskQueues = 36;

        /**
         * @brief queuesKeyRatio Defines how many queues can be used by a specific key.
         *        0.0 means only 1 queue, 1.0 means all queues are available for usage.
         *        Default value: 0.5 (half of the queues).
         */
        float queuesKeyRatio = 0.5;
    };

    Config parameters;
    ////////

    /**
     * @brief setAcceptorSocket Set Acceptor Socket, the acceptor socket is now in control of this class, deleting this class will delete the acceptor.
     * @param value acceptor socket
     */
    void setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream_Base> & value);


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

        _callbackConnectionRB onConnect = nullptr;
        _callbackConnectionRB onInitFail = nullptr;

        void * contextOnConnect = nullptr;
        void * contextOnInitFail = nullptr;

        std::string key;

        std::shared_ptr<Sockets::Socket_Stream_Base> clientSocket;
        char pad;
    };

    static void runner(void * data);
    static void stopper(void * data);
    static void acceptorTask(std::shared_ptr<void> data);

    void init();

    Mantids30::Threads::Pool::ThreadPool * m_pool = nullptr;
    std::shared_ptr<Sockets::Socket_Stream_Base> m_acceptorSocket;
    std::mutex m_runMutex;



};

}}}}

