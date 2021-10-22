#ifndef FASTRPC_H
#define FASTRPC_H

#include <cx2_hlp_functions/json.h>

#include <cx2_thr_threads/threadpool.h>
#include <cx2_thr_mutex/mutex_shared.h>
#include <cx2_thr_mutex/mutex.h>
#include <cx2_net_sockets/streamsocket.h>
#include <cx2_thr_safecontainers/map.h>

namespace CX2 { namespace RPC { namespace Fast {

struct sFastRPCOnConnectedMethod
{
    void (*fastRPCOnConnectedMethod)(const std::string &, void * obj);
    void * obj;
};

struct sFastRPCMethod
{
    /**
     * @brief Function pointer.
     */
    json (*rpcMethod)(void * obj, const std::string &key, const json & parameters);
    /**
     * @brief obj object to pass
     */
    void * obj;
};
struct sFastRPCParameters
{
    Network::Streams::StreamSocket *streamBack;
    uint32_t maxMessageSize;
    void * caller;
    Threads::Sync::Mutex_Shared * done;
    Threads::Sync::Mutex * mtSocket;
    std::string methodName;
    json payload;
    std::string key;
    uint64_t requestId;
};

class FastRPC_Connection : public CX2::Threads::Safe::Map_Element
{
public:
    FastRPC_Connection()
    {
       // lastReceivedData = 0;
        requestIdCounter = 1;
        terminated = false;
    }
    // Socket
    CX2::Network::Streams::StreamSocket * stream;
    Threads::Sync::Mutex * mtSocket;
    std::string key;

    // Request ID counter.
    uint64_t requestIdCounter;
    Threads::Sync::Mutex mtReqIdCt;

    // Answers:
    std::map<uint64_t,json> answers;
    std::map<uint64_t,uint8_t> executionStatus;
    std::mutex mtAnswers;
    std::condition_variable cvAnswers;
    std::set<uint64_t> pendingRequests;

    // Finalization:
    std::atomic<bool> terminated;
};

/**
 * @brief The FastRPC class: Bidirectional client-sync/server-async-thread-pooled no-auth RPC Manager
 */
class FastRPC
{
public:
    /**
     * @brief FastRPC This class is designed to persist between connections...
     * @param threadsCount
     * @param taskQueues
     */
    FastRPC(uint32_t threadsCount = 16, uint32_t taskQueues = 24);
    ~FastRPC();
    /**
     * @brief stop Stop the thread pool.
     */
    void stop();
    /**
     * @brief addMethod Add Method
     * @param methodName Method Name
     * @param rpcMethod Method function and Object
     */
    bool addMethod(const std::string & methodName, const sFastRPCMethod & rpcMethod);


    // Ping functions:
    /**
     * @brief sendPings Internal function to send pings to every connected peer
     */
    void sendPings();
    /**
     * @brief waitPingInterval Wait interval for pings
     * @return true if ping interval completed, false if a signal closed the wait interval (eg. FastRPC destroyed)
     */
    bool waitPingInterval();
    /**
     * @brief setPingInterval Set Ping interval
     * @param _intvl ping interval in seconds
     */
    void setPingInterval(uint32_t _intvl=20);
    /**
     * @brief getPingInterval Get ping interval
     * @return ping interval in seconds
     */
    uint32_t getPingInterval();

    /**
     * @brief processConnection Process Connection Stream and manage bidirectional events from each side (Q/A).
     *                          Additional security should be configured at the TLS Connections, like peer validation
     *                          and you may set up any other authentication mechanisms (like api keys) prior to the socket
     *                          management delegation or in each RPC function.
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param key Connection Name, used for running remote RPC methods.
     * @param keyDistFactor threadpool distribution usage by the key (0.5 = half, 1.0 = full, 0.NN = NN%)
     * @param callbackOnConnectedMethod On connect Method to be executed in background (new thread) when connection is accepted/processed.
     * @return 0 if remotely shutted down, or negative if connection error happened.
     */
    int processConnection(CX2::Network::Streams::StreamSocket * stream, const std::string & key, const sFastRPCOnConnectedMethod & callbackOnConnectedMethod = {nullptr,nullptr}, const float & keyDistFactor=1.0 );

    /**
     * @brief setTimeout Timeout in milliseconds to desist to put the execution task into the threadpool
     * @param value milliseconds, default is 2secs (2000).
     */
    void setQueuePushTimeoutInMS(const uint32_t &value = 2000);
    /**
     * @brief setMaxMessageSize Max JSON Size
     * @param value max bytes for reception/transmition json, default is 1M
     */
    void setMaxMessageSize(const uint32_t &value = 1024*1024);
    /**
     * @brief setRemoteExecutionTimeoutInMS Set the remote Execution Timeout for "runRemoteRPCMethod" function
     * @param value timeout in milliseconds, default is 5secs (5000).
     */
    void setRemoteExecutionTimeoutInMS(const uint32_t &value = 5000);
    /**
     * @brief runRemoteRPCMethod Run Remote RPC Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @param methodName Method Name
     * @param payload Function Payload
     * @param error Error Return
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    json runRemoteRPCMethod( const std::string &connectionKey, const std::string &methodName, const json &payload , json * error, bool retryIfDisconnected = true );
    /**
     * @brief runRemoteClose Run Remote Close Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    bool runRemoteClose( const std::string &connectionKey );

    /**
     * @brief getConnectionKeys Get keys from the current connections.
     * @return set of strings containing the unique keys
     */
    std::set<std::string> getConnectionKeys();

    /**
     * @brief checkConnectionKey Check if the given connection key does exist.
     * @param connectionKey connection key
     * @return true if exist, otherwise false.
     */
    bool checkConnectionKey( const std::string &connectionKey );

    //////////////////////////////////////////////////////////
    // For Internal use only:
    json runLocalRPCMethod(const std::string & methodName, const std::string &key, const json &payload, bool * found);

    void setRemoteExecutionDisconnectedTries(const uint32_t &value = 10);


    /**
     * @brief getReadTimeout Get R/W Timeout
     * @return W/R timeout in seconds
     */
    uint32_t getRWTimeout() const;
    /**
     * @brief setRWTimeout   This timeout will be setted on processConnection, call this function before that.
     *
     *                       Read timeout defines how long are we going to wait if no-data comes from a RPC connection socket,
     *                       we also send a periodic ping to avoid the connection to be closed due to RPC per-se inactivity, so
     *                       basically we are only going to close due to network issues.
     *
     *                       Write timeout is also important, since we are using blocking sockets, if the peer is full
     *                       just before having a network problem, the writes (including the ping one) may block the pinging process forever.
     *
     * @param _rwTimeout     W/R timeout in seconds (default: 40)
     */
    void setRWTimeout(uint32_t _rwTimeout = 40);

protected:
    virtual void eventUnexpectedAnswerReceived(FastRPC_Connection *connection, const std::string &answer);
    virtual void eventFullQueueDrop(sFastRPCParameters * params);
    virtual void eventRemotePeerDisconnected(const std::string &connectionKey, const std::string &methodName, const json &payload);
    virtual void eventRemoteExecutionTimedOut(const std::string &connectionKey, const std::string &methodName, const json &payload);

private:
    static void executeRPCTask(void * taskData);
    static void sendRPCAnswer(sFastRPCParameters * parameters, const std::string & answer, uint8_t execution);

    int processAnswer(FastRPC_Connection *connection);
    int processQuery(CX2::Network::Streams::StreamSocket * stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket);

    CX2::Threads::Safe::Map<std::string> connectionsByKeyId;

    std::atomic<uint32_t> queuePushTimeoutInMS,maxMessageSize, remoteExecutionTimeoutInMS, remoteExecutionDisconnectedTries;
    // Methods:
    // method name -> method.
    std::map<std::string,sFastRPCMethod> methods;
    Threads::Sync::Mutex_Shared smutexMethods;
    CX2::Threads::Pool::ThreadPool * threadPool;

    std::thread pinger;

    std::atomic<bool> finished;
    uint32_t pingIntvl, rwTimeout;
    std::mutex mtPing;
    std::condition_variable cvPing;
};

}}}
#endif // FASTRPC_H
