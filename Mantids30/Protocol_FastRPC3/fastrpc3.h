#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Sessions/session.h>

#include <Mantids30/API_Monolith/endpointshandler.h>
#include <Mantids30/Threads/threadpool.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <Mantids30/Threads/mutex.h>
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <Mantids30/Net_Sockets/callbacks_socket_tls_server.h>

#include <Mantids30/Net_Sockets/connector.h>
#include <Mantids30/Net_Sockets/listener.h>

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Threads/map.h>
#include <cstdint>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace FastRPC {

// TODO: online session tokens validator
/**
 * @brief The FastRPC3 class: Bidirectional client-sync/server-async-thread-pooled auth RPC Manager with sessions...
 */
class FastRPC3 : public Network::Sockets::Connector, public Network::Sockets::Listener
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

    enum eTaskExecutionStatus {
        EXEC_STATUS_ERR_GENERIC = 1,
        EXEC_STATUS_SUCCESS = 2,
        EXEC_STATUS_ERR_REMOTE_QUEUE_OVERFLOW = 3,
        EXEC_STATUS_ERR_METHOD_NOT_FOUND = 4
    };


    class SessionPTR {
    public:
        SessionPTR() = default;
        ~SessionPTR() = default;

        // Destroy a session
        bool destroy();

        // create a new session...
        std::shared_ptr<Sessions::Session> create(const DataFormat::JWT::Token &jwt);

        // Increment counter and getSharedPointer the object...
        std::shared_ptr<Sessions::Session> getSharedPointer();

    private:
        std::shared_ptr<Sessions::Session> session;
        std::mutex mt;
    };

    struct TaskParameters
    {
        ~TaskParameters()
        {
            if (extraTokenAuth)
                delete [] extraTokenAuth;
        }

        std::shared_ptr<Sockets::Socket_Stream> streamBack = nullptr;
        uint32_t maxMessageSize=0;
        void * caller = nullptr;
        FastRPC3::SessionPTR * sessionHolder = nullptr;
        std::shared_ptr<API::Monolith::Endpoints> methodsHandler;
        std::shared_ptr<DataFormat::JWT> jwtValidator = nullptr;
        Threads::Sync::Mutex_Shared * doneSharedMutex = nullptr;
        Threads::Sync::Mutex * socketMutex = nullptr;
        std::string methodName, remotePeerIPAddress, remotePeerTLSCommonName, userId, domain;
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
        std::shared_ptr<Sockets::Socket_Stream> stream = nullptr;
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

    struct RPC3CallbackDefinitions {

        enum eTokenValidationFailedErrors
        {
            TOKEN_VALIDATION_ERROR=1,
            EXTRATOKEN_VALIDATION_ERROR=2,
            EXTRATOKEN_IMPERSONATION_ERROR=3,
            EXTRATOKEN_NOTREQUIRED_ERROR=5,
            TOKEN_REVOKED=4,
        };

        void (*onMethodExecutionAuthorizerMissing)( void * context, TaskParameters * parameters ) = nullptr;
        void (*onMethodExecutionSessionMissing)( void * context, TaskParameters * parameters ) = nullptr;
        void (*onMethodExecutionStart)(void * context, TaskParameters * parameters,const json & payloadIn) = nullptr;
        void (*onMethodExecutionNotAuthorized)(void * context, TaskParameters * parameters,const json & reasons) = nullptr;
        void (*onMethodExecutionNotFound)(void * context, TaskParameters * parameters) = nullptr;
        void (*onMethodExecutionSuccess)(void * context, TaskParameters * parameters, const double & elapsedMS,const json & payloadOut ) = nullptr;
        void (*onMethodExecutionUnknownError)(void * context, TaskParameters * parameters ) = nullptr;
        void (*onImpersonationFailure)(void * context, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & authSlotId ) = nullptr;
        void (*onTokenValidationSuccess)(void * context, TaskParameters * parameters,  const std::string & jwtToken) = nullptr;
        void (*onTokenValidationFailure)(void * context, TaskParameters * parameters,  const std::string & jwtToken, eTokenValidationFailedErrors err ) = nullptr;
        void (*onProtocolUnexpectedResponse)(FastRPC3::Connection *connection, const std::string &answer) = nullptr;
        void (*onOutgoingTaskFailureDisconnectedPeer)(const std::string &connectionId, const std::string &methodName, const json &payload) = nullptr;
        void (*onIncomingTaskDroppedQueueFull)(FastRPC3::TaskParameters * params) = nullptr;
        void (*onOutgoingTaskFailureTimeout)(const std::string &connectionId, const std::string &methodName, const json &payload) = nullptr;

        void * context = nullptr;
        // Mantids30::Sessions::getReasonText(authReason) < - to obtain the auth reason.
    };

    class Config {
    public:
        Config(
                std::shared_ptr<DataFormat::JWT> _jwtValidator
            ) :  jwtValidator(_jwtValidator)
        {
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
        std::atomic<uint32_t> queuePushTimeoutInMS{2000};
        /**
         * @brief maxMessageSize Max JSON RAW Message Size
         */
        std::atomic<uint32_t> maxMessageSize{10*1024*1024};
        /**
         * @brief remoteExecutionTimeoutInMS Remote Execution Timeout for "runRemoteRPCMethod" function
         */
        std::atomic<uint32_t> remoteExecutionTimeoutInMS{5000};
        /**
         * @brief remoteExecutionDisconnectedTries
         */
        std::atomic<uint32_t> remoteExecutionDisconnectedTries{10};
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
        std::shared_ptr<API::Monolith::Endpoints> methodHandlers;
        /**
         * @brief jwtValidator JWT Validator.
         */
        std::shared_ptr<DataFormat::JWT> jwtValidator;

        // WARNING: this parameters should not be changed during execution:
        /**
         * @brief loginURL The Single Sign-On (SSO) URL used to obtain the JWT token for the session.
         *                 This URL is shared with the remote peer. If the remote peer wants to authenticate,
         *                 it should use this URL to initiate the authentication process.
         */
        std::string loginURL;
        /**
         * @brief returnURI The URI to be used as a callback in case of successful authentication.
         *                  Your application should intercept this URI and extract the JWT token from it.
         */
        std::string returnURI;

        /**
         * @brief ignoreSSLCertForSSO A flag to indicate whether SSL certificate validation should be ignored specifically for SSO login.
         *                            Set to `true` to bypass SSL certificate verification during the SSO login process (not recommended for production).
         *                            Default value is `false`, meaning SSL certificates are validated.
         */
        bool ignoreSSLCertForSSO = false;


        void setDefaultHandlers(std::shared_ptr<API::Monolith::Endpoints> x)
        {
            methodHandlers = x;
            defaultMethodsHandlers = x;
        }

    private:
        std::shared_ptr<API::Monolith::Endpoints> defaultMethodsHandlers;
    };

    class RemoteMethods {
    public:
        RemoteMethods(FastRPC3 * _parent,const std::string & _connectionId) : parent(_parent), connectionId(_connectionId) {}


        /**
         * @brief loginViaJWTToken Logs in to the remote peer using a JWT token.
         *
         * This function attempts to authenticate with the remote peer by providing a JWT (JSON Web Token).
         * The authentication process is completed successfully if the provided token is valid and accepted by the server.
         *
         * @param jwtToken A string containing the JSON Web Token (JWT) for authentication.
         * @param error A pointer to a json object that will store the result of the operation. If `error["success"]` is true,
         *              the operation completed successfully. Otherwise, additional details about the error may be provided
         *              in the `error` object.
         *
         * @return json Returns a JSON object containing the login response or authentication data if successful.
         */
        json loginViaJWTToken(const std::string &jwtToken, json *error = nullptr);

        /**
         * @brief logout Logs out the current user session.
         *
         * This function terminates the current authenticated session. If the logout is successful,
         * all associated session data will be cleared on both the client and server side.
         *
         * @param error A pointer to a json object that will store the result of the operation. If `error["success"]` is true,
         *              the operation completed successfully. Otherwise, additional details about the error may be provided
         *              in the `error` object.
         *
         * @return bool Returns true if the logout was successful; otherwise, returns false.
         */
        bool logout(json *error = nullptr);

        /**
         * @brief getSSOData Retrieves Single Sign-On (SSO) data parameters.
         *
         * This function fetches and returns SSO-related data or configuration parameters required for
         * authenticating or maintaining a session with the remote peer. The exact content of the returned data depends
         * on the specific SSO implementation being used.
         *
         * @param error A pointer to a json object that will store the result of the operation. If `error["success"]` is true,
         *              the operation completed successfully. Otherwise, additional details about the error may be provided
         *              in the `error` object.
         *
         * @return json Returns a JSON object containing SSO parameters if the request was successful.
         */
        json getSSOData(json *error = nullptr);

        /**
         * @brief Executes a remote task or method on a specified connection.
         *
         * This function allows you to call a remote method identified by `methodName`, sending it the provided `payload`.
         * If an error occurs during execution, details are stored in the `error` parameter. The function supports retries
         * if the connection is lost and can pass session commands based on user configuration.
         *
         * @param methodName The name of the remote method to execute.
         * @param payload A JSON object containing data or arguments needed by the remote method.
         * @param error A pointer to a JSON object where any error details will be stored if an error occurs.
         * @param retryIfDisconnected (Optional) Indicates whether to retry the operation if the connection is lost. Default: true.
         * @param passSessionCommands (Optional) Determines whether session commands should be passed along with the request. Default: false.
         * @param extraJWTTokenAuth (Optional) Additional authentication token for enhanced security during the remote method execution.
         *
         * @return A JSON object containing the answer from the remote method, or Json::nullValue if no answer is received, there's a timeout, or an error occurs.
         *
         * @note This function is thread-safe and can handle multiple connections simultaneously. It may return Json::nullValue due to network issues, timeouts, or other errors.
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
     * @class FastRPC3
     * @brief A persistent class designed to maintain state across connections.
     *
     * @details FastRPC3 handles task queuing and threading, enabling efficient
     * execution while maintaining authentication through a provided JWT validator.
     *
     * @param jwtValidator A shared pointer to the JWT validator for authentication.
     * @param threadsCount The number of threads to preload (default: 16).
     * @param taskQueues The maximum number of queued tasks per thread (default: 24).
     */
    FastRPC3(std::shared_ptr<DataFormat::JWT> jwtValidator,
             uint32_t threadsCount = 16,
             uint32_t taskQueues = 24);
    /**
     * @class FastRPC3
     * @brief A persistent class designed to maintain state across connections.
     *
     * @details FastRPC3 handles task queuing and threading, enabling efficient
     * execution while maintaining authentication through a provided JWT validator.
     *
     * @param threadsCount The number of threads to preload (default: 16).
     * @param taskQueues The maximum number of queued tasks per thread (default: 24).
     */
    FastRPC3(uint32_t threadsCount = 16, uint32_t taskQueues = 24);

    ~FastRPC3();
    /**
     * @brief stop Stop the thread pool.
     */
    void stop();

    int handleClientConnection(std::shared_ptr<Sockets::Socket_Stream> stream) override
    {
        return handleConnection(stream, false);
    }

    int handleServerConnection(std::shared_ptr<Sockets::Socket_Stream> stream) override
    {
        return handleConnection(stream, true);
    }

    /**
     * @brief handleConnection Process Connection Stream and manage bidirectional events from each side (Q/A).
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param serverMode true if the remote peer is a server
     * @param remotePair remote pair IP address detected by the acceptor
     * @return
     */
    int handleConnection(std::shared_ptr<Sockets::Socket_Stream> stream, bool remotePeerIsServer);


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
    RPC3CallbackDefinitions rpcCallbacks;
    /**
     * @brief parameters All configuration Parameters...
     */
    Config config;

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

    struct LoginReason
    {
        enum eReasons {
            INTERNAL_ERROR,
            SESSION_DUPLICATE,
            TOKEN_VALIDATED,
            TOKEN_FAILED
        };

        eReasons reason = INTERNAL_ERROR;

        Json::Value toJSONResponse() {
            Json::Value response;
            response["val"] = reason;

            switch (reason) {
            case INTERNAL_ERROR:
                response["txt"] = "An internal error occurred. Please try again later.";
                break;
            case SESSION_DUPLICATE:
                response["txt"] = "A duplicate session was detected. Ensure only one active session exists for each token.";
                break;
            case TOKEN_VALIDATED:
                response["txt"] = "The JWT token has been successfully validated.";
                break;
            case TOKEN_FAILED:
                response["txt"] = "JWT token validation failed. Please check the token configuration and try again.";
                break;
            default:
                response["txt"] = "An unknown reason caused the issue. Please contact support.";
                break;
            }

            return response;
        }
    };


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

        static void executeLocalTask(std::shared_ptr<void> taskData);
        static void getSSOData(std::shared_ptr<void> taskData);
        static void login(std::shared_ptr<void> taskData);
        static void logout(std::shared_ptr<void> taskData);
    };


    static void sendRPCAnswer(FastRPC3::TaskParameters * parameters, const std::string & answer, uint8_t executionStatus);

    int processIncomingAnswer(FastRPC3::Connection *connection);
    int processIncomingExecutionRequest(std::shared_ptr<Sockets::Socket_Stream> stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared *mtDone, Threads::Sync::Mutex *mtSocket, FastRPC3::SessionPTR *session);


    // TODO:
    /*    std::map<std::string,std::string> connectionIdToLogin;
    std::mutex connectionIdToLoginMutex;
    std::multimap<std::string,std::string> loginToConnectionKey;
    std::mutex loginToConnectionKeyMutex;*/


    // Methods:
    // method name -> method.
    // ? TP or
    //  static json passwordPublicDataToJSON(const uint32_t &slotId, const Sessions::Credential &publicData);
    //  void * overwriteObject;
    //Sessions::Domains m_defaultAuthDomain;


    /**
     * @brief Stores active connections indexed by a unique key identifier.
     */
    Mantids30::Threads::Safe::Map<std::string> m_connectionMapById;

    /**
     * @brief Thread pool for handling RPC method execution.
     */
    Mantids30::Threads::Pool::ThreadPool * m_threadPool;

    /**
     * @brief Background thread responsible for periodic pinging to maintain connection health.
     */
    std::thread m_pingerThread;

    /**
     * @brief Indicates whether the RPC system is in a finished state.
     */
    std::atomic<bool> m_isFinished;

    /**
     * @brief Mutex for synchronizing ping operations.
     */
    std::mutex m_pingMutex;

    /**
     * @brief Condition variable used to signal ping-related events.
     */
    std::condition_variable m_pingCondition;

    /**
     * @brief Handler for default RPC methods.
     */
    std::shared_ptr<Mantids30::API::Monolith::Endpoints> m_defaultMethodsHandlers = nullptr;

    /**
     * @brief Flag indicating whether the remote peer's common name is used as the connection ID.
     */
    bool m_usingRemotePeerCommonNameAsConnectionId = false;

};

}}}}



