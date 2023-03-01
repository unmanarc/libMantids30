#ifndef POOLTHREADEDACCEPTOR_H
#define POOLTHREADEDACCEPTOR_H

#include "socket_stream_base.h"

#include <Mantids29/Helpers/mem.h>
#include <Mantids29/Threads/threaded.h>
#include <Mantids29/Threads/threadpool.h>
#include <memory>

// TODO: statistics

namespace Mantids29 { namespace Network { namespace Sockets { namespace Acceptors {
/**
 * @brief The PoolThreaded class
 */
class PoolThreaded : public Mantids29::Threads::Threaded
{
public:

    typedef bool (*_callbackConnectionRB)(void *, Sockets::Socket_Stream_Base *, const char *, bool);
    typedef void (*_callbackConnectionRV)(void *, Sockets::Socket_Stream_Base *, const char *, bool);

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
     * @param obj object passed to all callbacks
     * @param _onInitFailed callback function on failed initialization (default nullptr -> none)
     * @param _onTimeOut callback function on time out (default nullptr -> none)
     * @param _onMaxConnectionsPerIP callback function when an ip reached the max number of connections (default nullptr -> none)
     */
    PoolThreaded(const std::shared_ptr<Sockets::Socket_Stream_Base> & acceptorSocket,
                    _callbackConnectionRB _onConnect,
                    void *obj=nullptr,
                    _callbackConnectionRB _onInitFailed=nullptr,
                    _callbackConnectionRV _onTimeOut=nullptr
                    );

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
    /**
     * Set callback when connection is fully established (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    void setCallbackOnConnect(_callbackConnectionRB _onConnect, void *obj);
    /**
     * Set callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    void setCallbackOnInitFail(_callbackConnectionRB _onInitFailed, void *obj);
    /**
     * Set callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    void setCallbackOnTimedOut(_callbackConnectionRV _onTimeOut, void *obj);

    /////////////////////////////////////////////////////////////////////////
    // TUNNING:
    /**
     * @brief getTimeoutMS Get the timeout in milliseconds
     * @return value timeout to cease to try to insert the task in a queue
     */
    uint32_t getTimeoutMS() const;
    /**
     * @brief setTimeoutMS Set the timeout in milliseconds
     * @param value timeout to cease to try to insert the task in a queue
     */
    void setTimeoutMS(const uint32_t &value);
    ////////
    /**
     * @brief getThreadsCount Get how many threads will be used (call before start)
     * @return thread count
     */
    uint32_t getThreadsCount() const;
    /**
     * @brief setThreadsCount Set how many threads will be used (call before start)
     * @param value thread count
     */
    void setThreadsCount(const uint32_t &value);
    ////////
    /**
     * @brief getTaskQueues Get how many queues to store tasks, each queue handle 100 tasks in wait mode.
     * @return task queues count
     */
    uint32_t getTaskQueues() const;
    /**
     * @brief setTaskQueues Set how many queues to store tasks, each queue handle 100 tasks in wait mode.
     * @param value task queues count
     */
    void setTaskQueues(const uint32_t &value);
    ////////
    /**
     * @brief getQueuesKeyRatio Get how many queues can be used by some key
     * @return value number from (0-1], 0.0 means 1 queue, and 1 is for all queues, default value: 0.5 (half)
     */
    float getQueuesKeyRatio() const;
    /**
     * @brief setQueuesKeyRatio Set how many queues can be used by some key (in this case, key is the source ip address)
     *                          using all queues means that there is no mechanism to prevent saturation.
     * @param value number from (0-1], 0.0 means 1 queue, and 1 is for all queues, default value: 0.5 (half)
     */
    void setQueuesKeyRatio(float value);
    /**
     * @brief setAcceptorSocket Set Acceptor Socket, the acceptor socket is now in control of this class, deleting this class will delete the acceptor.
     * @param value acceptor socket
     */
    void setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream_Base> & value);

private:

    struct sAcceptorTaskData
    {
        ~sAcceptorTaskData()
        {
            if (clientSocket)
            {
                clientSocket->shutdownSocket();
                delete clientSocket;
                clientSocket = nullptr;
            }
            isSecure = false;
            ZeroBArray(remotePair);
        }

        bool (*onConnect)(void *,Sockets::Socket_Stream_Base *, const char *,bool);
        bool (*onInitFail)(void *,Sockets::Socket_Stream_Base *, const char *,bool);
        void *objOnConnect, *objOnInitFail;

        std::string key;

        void * obj;
        Sockets::Socket_Stream_Base * clientSocket;
        char remotePair[INET6_ADDRSTRLEN];
        bool isSecure;
        char pad;
    };

    static void runner(void * data);
    static void stopper(void * data);
    static void acceptorTask(void * data);

    void init();

    Mantids29::Threads::Pool::ThreadPool * pool;
    std::shared_ptr<Sockets::Socket_Stream_Base> acceptorSocket;

    _callbackConnectionRB onConnect;
    _callbackConnectionRB onInitFail;
    _callbackConnectionRV onTimedOut;

    void *objOnConnect, *objOnInitFail, *objOnTimedOut;

    float queuesKeyRatio;
    uint32_t timeoutMS;
    uint32_t threadsCount;
    uint32_t taskQueues;
};

}}}}

#endif // POOLTHREADEDACCEPTOR_H
