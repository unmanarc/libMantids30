#ifndef FASTRPC_H
#define FASTRPC_H

#include <json/json.h>

#include <cx2_thr_threads/threadpool.h>
#include <cx2_thr_mutex/mutex_shared.h>
#include <cx2_thr_mutex/mutex.h>
#include <cx2_net_sockets/streamsocket.h>
#include <cx2_thr_mutex_map/map.h>

namespace CX2 { namespace RPC { namespace Fast {

struct sFastRPCMethod
{
    /**
     * @brief Function pointer.
     */
    Json::Value (*rpcMethod)(void * obj, const Json::Value & parameters);
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
    Json::Value payload;
    //std::string key;
    uint64_t requestId;
};

class FastRPC_Connection : public CX2::Threads::Safe::Map_Element
{
public:
    FastRPC_Connection()
    {
        requestIdCounter = 1;
        terminated = false;
    }
    // Socket
    CX2::Network::Streams::StreamSocket * stream;
    Threads::Sync::Mutex * mtSocket;

    // Request ID counter.
    uint64_t requestIdCounter;
    Threads::Sync::Mutex mtReqIdCt;

    // Answers:
    std::map<uint64_t,Json::Value> answers;
    std::mutex mtAnswers;
    std::condition_variable cvAnswers;
    std::set<uint64_t> pendingRequests;

    // Finalization:
    bool terminated;
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
    /**
     * @brief processConnection Process Connection Stream and manage bidirectional events from each side (Q/A).
     *                          Additional security should be configured at the TLS Connections, like peer validation
     *                          and you may set up any other authentication mechanisms (like api keys) prior to the socket
     *                          management delegation or in each RPC function.
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param key Connection Name, used for running remote RPC methods.
     * @param priority threadpool distribution usage (0.5 = half, 1.0 = full, 0.NN = NN%)
     * @return 0 if remotely shutted down, or negative if connection error happened.
     */
    int processConnection(CX2::Network::Streams::StreamSocket * stream, const std::string & key, const float & priority=1.0 );
    /**
     * @brief setTimeout Timeout in milliseconds to desist to put the execution task into the threadpool
     * @param value milliseconds, default is 2secs (2000).
     */
    void setQueuePushTimeoutInMS(const uint32_t &value = 2000);
    /**
     * @brief setMaxMessageSize Max JSON Size
     * @param value max bytes for reception/transmition json, default is 256K
     */
    void setMaxMessageSize(const uint32_t &value = 256*1024);
    /**
     * @brief setRemoteExecutionTimeoutInMS Set the remote Execution Timeout for "runRemoteRPCMethod" function
     * @param value timeout in milliseconds, default is 2secs (2000).
     */
    void setRemoteExecutionTimeoutInMS(const uint32_t &value = 2000);
    /**
     * @brief runRemoteRPCMethod Run Remote RPC Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @param methodName Method Name
     * @param payload Function Payload
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    Json::Value runRemoteRPCMethod( const std::string &connectionKey, const std::string &methodName, const Json::Value &payload );

    //////////////////////////////////////////////////////////
    // For Internal use only:
    Json::Value runLocalRPCMethod(const std::string & methodName, const Json::Value &payload);

protected:
    virtual void eventUnexpectedAnswerReceived(FastRPC_Connection *connection, const std::string &answer);
    virtual void eventFullQueueDrop(sFastRPCParameters * params);
    virtual void eventRemotePeerDisconnected(const std::string &connectionKey, const std::string &methodName, const Json::Value &payload);
    virtual void eventRemoteExecutionTimedOut(const std::string &connectionKey, const std::string &methodName, const Json::Value &payload);

private:
    static void executeRPCTask(void * taskData);
    static void sendRPCAnswer(sFastRPCParameters * parameters, const std::string & answer);

    int processAnswer(FastRPC_Connection *connection);
    int processQuery(CX2::Network::Streams::StreamSocket * stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket);

    CX2::Threads::Safe::Map<std::string> connectionsByKeyId;

    std::atomic<uint32_t> queuePushTimeoutInMS,maxMessageSize, remoteExecutionTimeoutInMS;
    // Methods:
    // method name -> method.
    std::map<std::string,sFastRPCMethod> methods;
    Threads::Sync::Mutex_Shared smutexMethods;
    CX2::Threads::Pool::ThreadPool * threadPool;
};

}}}
#endif // FASTRPC_H
