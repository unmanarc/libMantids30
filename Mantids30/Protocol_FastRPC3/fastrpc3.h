#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Auth/session.h>

#include <Mantids30/Auth/domains.h>
#include <Mantids30/API_Monolith/methodshandler.h>

#include <Mantids30/Threads/threadpool.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <Mantids30/Threads/mutex.h>
#include <Mantids30/Net_Sockets/socket_stream_base.h>
#include <Mantids30/Net_Sockets/callbacks_socket_tls_server.h>
#include <Mantids30/Net_Sockets/socket_tls_listennerandconnector_base.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Threads/map.h>
#include <cstdint>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace FastRPC {

// TODO: online session tokens validator
/**
 * @brief The FastRPC3 class: Bidirectional client-sync/server-async-thread-pooled auth RPC Manager with sessions...
 */
class FastRPC3 : public Network::Sockets::Socket_TLS_ListennerAndConnector_Base
{
public:
    ////////////////////////////////
    //     SUBCLASS DEFINTIONS:
    ////////////////////////////////

    enum eConnectionHandlerReturn {
        CONNECTION_SHUTDOWN_OK=0,
        CONNECTION_READ_TIMEOUT=-101,
        CONNECTION_INVALID_PROTOCOL=-100,
        CONNECTION_FAILED_READING_REQUESTID=-1,
        CONNECTION_FAILED_READING_FLAGS=-2,
        CONNECTION_FAILED_READING_METHOD_NAME=-3,
        CONNECTION_FAILED_READING_PAYLOAD=-4,
        CONNECTION_FAILED_READING_EXTRAAUTH=-5,
        CONNECTION_FAILED_PARSING_PAYLOAD=-6,
        CONNECTION_CONTINUE=1
    };

    enum eExecutionFlags {
        EXEC_FLAG_EMPTY = 0,
        EXEC_FLAG_NORMAL = 1,
        EXEC_FLAG_EXTRAAUTH = 2
    };

    enum eTaskExecutionErrors {
        EXEC_SUCCESS = 0,
        EXEC_ERR_PAYLOAD_TOO_LARGE = 1,
        EXEC_ERR_PEER_NOT_FOUND = 2,
        EXEC_ERR_DATA_TRANSMISSION_FAILURE = 3,
        EXEC_ERR_TIMEOUT = 4,
        EXEC_ERR_REMOTE_QUEUE_OVERFLOW = 5,
        EXEC_ERR_METHOD_NOT_FOUND = 6,
        EXEC_ERR_CONNECTION_LOST = 7,
        EXEC_ERR_UNKNOWN = 99,
    };

    class SessionPTR {
    public:
        SessionPTR();
        ~SessionPTR();

        // Destroy a session
        bool destroy();

        // create a new session...
        std::shared_ptr<Auth::Session> create();

        // Increment counter and getSharedPointer the object...
        std::shared_ptr<Auth::Session> getSharedPointer();

    private:
        std::shared_ptr<Auth::Session> session;
        std::mutex mt;
    };

    struct TaskParameters
    {
        ~TaskParameters()
        {
            if (extraTokenAuth)
                delete [] extraTokenAuth;
        }

        Network::Sockets::Socket_Stream_Base *streamBack = nullptr;
        uint32_t maxMessageSize=0;
        void * caller = nullptr;
        FastRPC3::SessionPTR * sessionHolder = nullptr;
        Mantids30::API::Monolith::MethodsHandler *methodsHandler;
        DataFormat::JWT * jwtValidator = nullptr;
        Threads::Sync::Mutex_Shared * doneSharedMutex = nullptr;
        Threads::Sync::Mutex * socketMutex = nullptr;
        std::string methodName, remotePeerIPAddress, remotePeerTLSCommonName, userId;
        char * extraTokenAuth = nullptr;
        json payload;
        uint64_t requestId = 0;
        void * callbacks = nullptr;
    };

    class Connection : public Mantids30::Threads::Safe::MapItem
    {
    public:
        Connection()
        {
            terminated = false;
        }

        void * callbacks = nullptr;

        // Socket
        Mantids30::Network::Sockets::Socket_Stream_Base * stream = nullptr;
        Threads::Sync::Mutex * socketMutex = nullptr;
        std::string key;

        // Request ID counter.
        uint64_t requestIdCounter = 1;
        Threads::Sync::Mutex mtReqIdCt;

        // Answers:
        std::map<uint64_t,json> answers;
        std::mutex answersMutex;
        std::condition_variable answersCondition;

        // Execution Status:
        std::map<uint64_t,uint8_t> executionStatus;

        // Pending Requests:
        std::set<uint64_t> pendingRequests;

        // Finalization:
        std::atomic<bool> terminated;

    };

    struct CallbackDefinitions {

        enum eTokenValidationFailedErrors
        {
            TOKEN_VALIDATION_ERROR=1,
            EXTRATOKEN_VALIDATION_ERROR=2,
            EXTRATOKEN_IMPERSONATION_ERROR=3,
            TOKEN_REVOKED=4,
        };


        void (*CB_MethodExecution_RequiredAuthorizerNotProvided)( void * obj, TaskParameters * parameters ) = nullptr;
        void (*CB_MethodExecution_RequiredSessionNotProvided)( void * obj, TaskParameters * parameters ) = nullptr;
        void (*CB_MethodExecution_Starting)(void * obj, TaskParameters * parameters,const json & payloadIn) = nullptr;
        void (*CB_MethodExecution_NotAuthorized)(void * obj, TaskParameters * parameters,const json & reasons) = nullptr;
        void (*CB_MethodExecution_MethodNotFound)(void * obj, TaskParameters * parameters) = nullptr;
        //void (*CB_MethodExecution_DomainNotFound)(void * obj, TaskParameters * parameters) = nullptr;
        void (*CB_MethodExecution_ExecutedOK)(void * obj, TaskParameters * parameters, const double & elapsedMS,const json & payloadOut ) = nullptr;
        void (*CB_MethodExecution_UnknownError)(void * obj, TaskParameters * parameters ) = nullptr;

        void (*CB_ImpersonationFailed)(void * obj, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & authSlotId ) = nullptr;

        void (*CB_TokenValidation_OK)(void * obj, TaskParameters * parameters,  const std::string & jwtToken) = nullptr;
        void (*CB_TokenValidation_Failed)(void * obj, TaskParameters * parameters,  const std::string & jwtToken, eTokenValidationFailedErrors err ) = nullptr;

        void (*CB_Protocol_UnexpectedAnswerReceived)(FastRPC3::Connection *connection, const std::string &answer) = nullptr;
        void (*CB_OutgoingTask_FailedExecutionOnDisconnectedPeer)(const std::string &connectionId, const std::string &methodName, const json &payload) = nullptr;
        void (*CB_IncommingTask_DroppingOnFullQueue)(FastRPC3::TaskParameters * params) = nullptr;
        void (*CB_OutgoingTask_FailedExecutionTimedOut)(const std::string &connectionId, const std::string &methodName, const json &payload) = nullptr;

        void * obj = nullptr;
        // Mantids30::Auth::getReasonText(authReason) < - to obtain the auth reason.
    };

    class ParametersDefinitions {
    public:
        ParametersDefinitions(
                API::Monolith::MethodsHandler *_methodsHandlers,
                DataFormat::JWT * _jwtValidator
            ) : methodHandlers(_methodsHandlers),  jwtValidator(_jwtValidator)
        {
            this->defaultMethodsHandlers = methodHandlers;

            maxMessageSize = 10*1024*1024;
            queuePushTimeoutInMS = 2000;
            remoteExecutionTimeoutInMS = 5000;
            remoteExecutionDisconnectedTries = 10;
        }
        ~ParametersDefinitions()
        {
            if ( defaultMethodsHandlers != methodHandlers )
                delete methodHandlers;
        }
        /**
         * @brief pingIntvl Ping Interval (in seconds) - Set before connect / not thread safe.
         */
        uint32_t pingIntervalInSeconds = 20;
        /**
         * @brief keyDistFactor float value from 0 to 1, 0 is no threads used, and 1 to allow in every thread. - Set before connect / not thread safe.
         */
        float keyDistFactor = 1.0;
        /**
         * @brief queuePushTimeoutInMS Queue Timeout in milliseconds to desist to put the execution task into the threadpool
         */
        std::atomic<uint32_t> queuePushTimeoutInMS;
        /**
         * @brief maxMessageSize Max JSON RAW Message Size
         */
        std::atomic<uint32_t> maxMessageSize;
        /**
         * @brief remoteExecutionTimeoutInMS Remote Execution Timeout for "runRemoteRPCMethod" function
         */
        std::atomic<uint32_t> remoteExecutionTimeoutInMS;
        /**
         * @brief remoteExecutionDisconnectedTries
         */
        std::atomic<uint32_t> remoteExecutionDisconnectedTries;
        /**
         * @brief rwTimeout This timeout will be setted on processConnection, call this function before that.
         *                  Read timeout defines how long are we going to wait if no-data comes from a RPC connection socket,
         *                  we also send a periodic ping to avoid the connection to be closed due to RPC per-se inactivity, so
                            basically we are only going to close due to network issues.
         *
                            Write timeout is also important, since we are using blocking sockets, if the peer is full
                            just before having a network problem, the writes (including the ping one) may block the pinging process forever.
         */
        uint32_t rwTimeoutInSeconds = 40;
        /**
         * @brief methodHandlers current Methods Manager
         */
        API::Monolith::MethodsHandler *methodHandlers;
        /**
         * @brief jwtValidator JWT Validator.
         */
        DataFormat::JWT * jwtValidator;
    private:
        API::Monolith::MethodsHandler *defaultMethodsHandlers;
    };

    class RemoteMethods {
    public:
        RemoteMethods(FastRPC3 * _parent,const std::string & _connectionId) : parent(_parent), connectionId(_connectionId) {}
        /**
         * @brief loginViaJWTToken
         * @param connectionId
         * @param jwtToken
         * @param error
         * @return
         */
        json loginViaJWTToken( const std::string & jwtToken, json *error = nullptr );
        /**
         * @brief logout
         * @param connectionId
         * @param error
         * @return
         */
        bool logout(   json *error = nullptr  );
        /**
         * @brief runRemoteRPCMethod Run Remote RPC Method
         * @param connectionId Connection ID (this class can thread-safe handle multiple connections at time)
         * @param methodName Method Name
         * @param payload Function Payload
         * @param error Error Return
         * @return Answer, or Json::nullValue if answer is not received or if timed out.
         */
        json executeTask(const std::string &methodName,
                         const json &payload,
                         json * error,
                         bool retryIfDisconnected = true,
                         bool passSessionCommands = false,
                         const std::string &extraJWTTokenAuth = ""
                         );
        /**
         * @brief runRemoteClose Run Remote Close Method
         * @param connectionId Connection ID (this class can thread-safe handle multiple connections at time)
         * @return Answer, or Json::nullValue if answer is not received or if timed out.
         */
        bool close( );

    private:
        FastRPC3 * parent;
        std::string connectionId;
    };

    RemoteMethods remote(const std::string & connectionId) { return RemoteMethods(this,connectionId); }

    ////////////////////////////////
    /**
     * @brief FastRPC3 This class is designed to persist between connections...
     * @param appName the application name to be used on the authentication server.
     * @param threadsCount number of preloaded threads
     * @param taskQueues number of max queued tasks for each thread
     */
    FastRPC3(DataFormat::JWT *jwtValidator, const std::string &appName = "", uint32_t threadsCount = 16, uint32_t taskQueues = 24);
    ~FastRPC3();
    /**
     * @brief stop Stop the thread pool.
     */
    void stop();

    /**
     * @brief handleConnection Process Connection Stream and manage bidirectional events from each side (Q/A).
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param serverMode true if the remote peer is a server
     * @param remotePair remote pair IP address detected by the acceptor
     * @return
     */
    int handleConnection(Mantids30::Network::Sockets::Socket_TLS * stream, bool remotePeerIsServer, const char * remotePair);


    // TODO: runRemoteRPCMethod for sending message to an specific connected user...
    // TODO: runRemoteRPCMethod for sending message to all connected users...
    // TODO: runRemoteRPCMethod for sending message to all connected users from specific role...
    // TODO: message persistence for non-connected users
    // TODO: list current sessions


    /**
     * @brief listActiveConnectionIds Get keys from the current connections.
     * @return set of strings containing the unique keys
     */
    std::set<std::string> listActiveConnectionIds();
    /**
     * @brief doesConnectionExist Check if the given connection key does exist.
     * @param connectionId connection key
     * @return true if exist, otherwise false.
     */
    bool doesConnectionExist( const std::string &connectionId );

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // For Internal use only:
    /**
     * @brief pingAllActiveConnections Internal function to send pings to every connected peer
     */
    void pingAllActiveConnections();
    /**
     * @brief waitPingInterval Wait interval for pings
     * @return true if ping interval completed, false if a signal closed the wait interval (eg. FastRPC3 destroyed)
     */
    bool waitPingInterval();

    /**
     * @brief callbacks This is where you define the callbacks before using this class...
     */
    CallbackDefinitions m_callbacks;
    /**
     * @brief parameters All configuration Parameters...
     */
    ParametersDefinitions m_parameters;

    /**
     * @brief isUsingRemotePeerCommonNameAsConnectionId Get if using the Common Name as the connection ID
     * @return true if using the Common Name as the connection ID
     */
    bool isUsingRemotePeerCommonNameAsConnectionId() const;
    /**
     * @brief setUsingRemotePeerCommonNameAsConnectionId Set to use or not the Common Name as the connection Id.
     * @param newUseCNAsServerKey true to use the common name as the connection id.
     */
    void setUsingRemotePeerCommonNameAsConnectionId(const bool & newUseCNAsServerKey);

private:

    class LocalRPCTasks {
    public:
        enum eExecuteLocalTaskRetCodes
        {
            ELT_RET_SUCCESS = 0,
            ELT_RET_TOKENFAILED = 1,
            ELT_RET_INVALIDIMPERSONATOR = 2,
            ELT_RET_REQSESSION = 3,
            ELT_RET_METHODNOTIMPLEMENTED = 404,
            ELT_RET_NOTAUTHORIZED = 403,
            ELT_RET_INTERNALERROR = 500
        };


        static void executeLocalTask(void * taskData);
        static void login(void * taskData);
        static void logout(void * taskData);
    };


    static void sendRPCAnswer(FastRPC3::TaskParameters * parameters, const std::string & answer, uint8_t execution);

    int processIncommingAnswer(FastRPC3::Connection *connection);
    int processIncommingExecutionRequest(Network::Sockets::Socket_TLS *stream,
                     const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket, FastRPC3::SessionPTR *session
                     );

    Mantids30::Threads::Safe::Map<std::string> m_connectionMapById;
    
    
  //  static json passwordPublicDataToJSON(const uint32_t &slotId, const Auth::Credential &publicData);

    // TODO:
/*    std::map<std::string,std::string> m_connectionIdToLogin;
    std::mutex m_connectionIdToLoginMutex;
    std::multimap<std::string,std::string> m_loginToConnectionKey;
    std::mutex m_loginToConnectionKeyMutex;*/


    // Methods:
    // method name -> method.
    // ? TP or
    Mantids30::Threads::Pool::ThreadPool * m_threadPool;

    std::thread m_pingerThread;
  //  void * overwriteObject;

    std::atomic<bool> m_isFinished;
    std::mutex m_pingMutex;
    std::condition_variable m_pingCondition;

    //Auth::Domains m_defaultAuthDomain;
    Mantids30::API::Monolith::MethodsHandler m_defaultMethodsHandlers;

    bool m_usingRemotePeerCommonNameAsConnectionId = false;
};

}}}}


