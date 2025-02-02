#pragma once

#include <list>
#include <map>
#include <memory>
#include <thread>
#include <condition_variable>

#include "acceptor_thread.h"
#include "socket_stream_base.h"

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
     * @param _onMaxConnectionsPerIP callback function when an ip reached the max number of connections (default nullptr -> none)
     */
    MultiThreaded(const std::shared_ptr<Socket_Stream_Base> &acceptorSocket,
                    _callbackConnectionRB _onConnect,
                    void * context=nullptr,
                    _callbackConnectionRB _onInitFailed=nullptr,
                    _callbackConnectionRV _onTimeOut=nullptr,
                    _callbackConnectionLimit _onMaxConnectionsPerIP=nullptr
                    );
    /**
     * Destructor
     * WARN: when you finalize this class, the listening socket is closed. please open another one (don't reuse it)
     */
    ~MultiThreaded();


    class Callbacks
    {
    public:

        void setAllContexts(void * context){
            contextOnConnect = context;
            contextOnInitFail = context;
            contextOnTimedOut = context;
            contextOnMaxConnectionsPerIP = context;
        }

        // Callbacks:
        _callbackConnectionRB onConnect = nullptr;
        _callbackConnectionRB onInitFail = nullptr;
        _callbackConnectionRV onTimedOut = nullptr;
        _callbackConnectionLimit onMaxConnectionsPerIP = nullptr;

        void * contextOnConnect = nullptr;
        void * contextOnInitFail = nullptr;
        void * contextOnTimedOut = nullptr;
        void * contextOnMaxConnectionsPerIP = nullptr;
    };

    Callbacks callbacks;


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
    void setAcceptorSocket(const std::shared_ptr<Socket_Stream_Base> &acceptorSocket);
    /**
     * Do accept on the acceptor socket.
     * @return true if we can still accept a new connection
     */
    bool acceptClient();
    /**
     * Finalize/Catch the client thread element (when it finishes).
     */
    bool finalizeThreadElement(std::shared_ptr<SAThread> x);
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


    // TODO: pasar tdo le tunning a Config / paramters;
private:
    static void thread_streamaccept(const std::shared_ptr<MultiThreaded> &tc);

    bool processClient(std::shared_ptr<Sockets::Socket_Stream_Base> clientSocket, std::shared_ptr<SAThread> clientThread);

    uint32_t incrementIPUsage(const std::string & ipAddr);
    void decrementIPUsage(const std::string & ipAddr);

    void init();

    bool initialized=false;
        bool finalized=false;
    std::shared_ptr<Sockets::Socket_Stream_Base> acceptorSocket;
    std::list<std::shared_ptr<SAThread>> threadList;
    std::map<std::string, uint32_t> connectionsPerIP;

    // thread objects:
    std::thread acceptorThread;

    //
    uint32_t maxConcurrentClients, maxWaitMSTime, maxConnectionsPerIP;
    std::mutex mutex_clients;
    std::condition_variable cond_clients_empty, cond_clients_notfull;
};


}}}}
