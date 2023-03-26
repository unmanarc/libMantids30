#ifndef FastRPC2_H
#define FastRPC2_H


#include <Mantids29/Helpers/json.h>
#include <Mantids29/Auth/session.h>

#include <Mantids29/Auth_Remote/loginrpcclientcbm.h>
#include <Mantids29/Auth/domains.h>
#include <Mantids29/API_Core/methodshandler.h>

#include <Mantids29/Threads/threadpool.h>
#include <Mantids29/Threads/mutex_shared.h>
#include <Mantids29/Threads/mutex.h>
#include <Mantids29/Net_Sockets/socket_stream_base.h>
#include <Mantids29/Net_Sockets/callbacks_socket_tls_server.h>
#include <Mantids29/Net_Sockets/socket_tls_listennerandconnector_base.h>
#include <Mantids29/Threads/map.h>
#include <cstdint>
#include <memory>

namespace Mantids29 { namespace Network { namespace Protocols { namespace FastRPC {

/*
 * Server Example code:
 *
 * FastRPC2 vsrpc(obj,"MYAPP");
 * vsrpc.getCurrentLoginRPCClient()-> ...
 * vsrpc.getCurrentLoginRPCClient()->start();
 *
 */

/**
 * @brief The FastRPC2 class: Bidirectional client-sync/server-async-thread-pooled auth RPC Manager with sessions...
 */
class FastRPC2 : public Network::Sockets::Socket_TLS_ListennerAndConnector_Base
{
public:
    ////////////////////////////////
    //     SUBCLASS DEFINTIONS:
    ////////////////////////////////

    class SessionPTR {
    public:
        SessionPTR();
        ~SessionPTR();

        // Destroy a session
        bool destroy();

        // create a new session...
        std::shared_ptr<Authentication::Session> create(const std::string &appName);

        // Increment counter and get the object...
        std::shared_ptr<Authentication::Session> get();

    private:
        std::shared_ptr<Authentication::Session> session;
        std::mutex mt;
    };

    struct TaskParameters
    {
        Network::Sockets::Socket_Stream_Base *streamBack;
        uint32_t maxMessageSize;
        void * caller;
        FastRPC2::SessionPTR * sessionHolder;
        API::MethodsHandler *currentMethodsHandlers;
        Authentication::Domains * currentAuthDomains;
        Threads::Sync::Mutex_Shared * done;
        Threads::Sync::Mutex * mtSocket;
        std::string methodName, ipAddr, cn;
        json payload;
        uint64_t requestId;
        void * callbacks;
    };

    class Connection : public Mantids29::Threads::Safe::MapItem
    {
    public:
        Connection()
        {
            requestIdCounter = 1;
            terminated = false;
        }

        void * callbacks;

        // Socket
        Mantids29::Network::Sockets::Socket_Stream_Base * stream;
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

    struct CallbackDefinitions {
        CallbackDefinitions()
        {
            CB_MethodExecution_RequiredSessionNotProvided = nullptr;
            CB_MethodExecution_ValidatedTemporaryAuthFactor = nullptr;
            CB_MethodExecution_FailedValidationOnTemporaryAuthFactor = nullptr;
            CB_MethodExecution_Starting = nullptr;
            CB_MethodExecution_NotAuthorized = nullptr;
            CB_MethodExecution_MethodNotFound = nullptr;
            CB_MethodExecution_DomainNotFound = nullptr;
            CB_MethodExecution_ExecutedOK = nullptr;
            CB_MethodExecution_UnknownError = nullptr;

            CB_Login_HalfAuthenticationRequireNextFactor = nullptr;
            CB_Login_LoggedIn = nullptr;
            CB_Login_InvalidDomain = nullptr;
            CB_Login_AuthenticationFailed = nullptr;

            CB_PasswordChange_RequestedOK = nullptr;
            CB_PasswordChange_RequestFailed = nullptr;
            CB_PasswordChange_BadCredentials = nullptr;
            CB_PasswordChange_InvalidDomain = nullptr;

            CB_PasswordValidation_OK = nullptr;
            CB_PasswordValidation_Failed = nullptr;
            CB_PasswordValidation_InvalidDomain = nullptr;

            CB_RemotePeer_UnexpectedAnswerReceived = nullptr;
            CB_RemotePeer_Disconnected = nullptr;
            CB_Incomming_DroppingOnFullQueue=nullptr;
            CB_Outgoing_ExecutionTimedOut=nullptr;
        }

        void (*CB_MethodExecution_RequiredSessionNotProvided)( void * obj, TaskParameters * parameters );
        void (*CB_MethodExecution_ValidatedTemporaryAuthFactor)(void * obj, TaskParameters * parameters,const uint32_t & passIdx, const Mantids29::Authentication::Reason & authReason);
        void (*CB_MethodExecution_FailedValidationOnTemporaryAuthFactor)(void * obj, TaskParameters * parameters,const uint32_t & passIdx, const Mantids29::Authentication::Reason & authReason);
        void (*CB_MethodExecution_Starting)(void * obj, TaskParameters * parameters,const json & payloadIn);
        void (*CB_MethodExecution_NotAuthorized)(void * obj, TaskParameters * parameters,const json & reasons);
        void (*CB_MethodExecution_MethodNotFound)(void * obj, TaskParameters * parameters);
        void (*CB_MethodExecution_DomainNotFound)(void * obj, TaskParameters * parameters);
        void (*CB_MethodExecution_ExecutedOK)(void * obj, TaskParameters * parameters, const double & elapsedMS,const json & payloadOut );
        void (*CB_MethodExecution_UnknownError)(void * obj, TaskParameters * parameters );

        void (*CB_Login_HalfAuthenticationRequireNextFactor)(void * obj, TaskParameters * parameters,const json & nextFactorResponse );
        void (*CB_Login_LoggedIn)(void * obj, TaskParameters * parameters,const json & nextFactorResponse, const std::string & user, const std::string & domain );
        void (*CB_Login_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain );
        void (*CB_Login_AuthenticationFailed)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const Mantids29::Authentication::Reason & authReason );

        void (*CB_PasswordChange_RequestedOK)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const uint32_t & credIdx );
        void (*CB_PasswordChange_RequestFailed)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const uint32_t & credIdx );
        void (*CB_PasswordChange_BadCredentials)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const uint32_t & credIdx , const Mantids29::Authentication::Reason & authReason);
        void (*CB_PasswordChange_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain, const uint32_t & credIdx );

        void (*CB_PasswordValidation_OK)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const uint32_t & credIdx );
        void (*CB_PasswordValidation_Failed)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const uint32_t & credIdx, const Mantids29::Authentication::Reason & authReason );
        void (*CB_PasswordValidation_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain, const uint32_t & credIdx );

        void (*CB_RemotePeer_UnexpectedAnswerReceived)(FastRPC2::Connection *connection, const std::string &answer);
        void (*CB_RemotePeer_Disconnected)(const std::string &connectionKey, const std::string &methodName, const json &payload);
        void (*CB_Incomming_DroppingOnFullQueue)(FastRPC2::TaskParameters * params);
        void (*CB_Outgoing_ExecutionTimedOut)(const std::string &connectionKey, const std::string &methodName, const json &payload);


        void * obj;
        // Mantids29::Authentication::getReasonText(authReason) < - to obtain the auth reason.
    };

    class ParametersDefinitions {
    public:
        ParametersDefinitions(
                Authentication::LoginRPCClientCBM * currentLoginRPCClient,
                API::MethodsHandler *currentMethodsHandlers,
                Authentication::Domains * currentAuthDomains
                )
        {
            this->currentLoginRPCClient = currentLoginRPCClient;
            this->currentAuthDomains = currentAuthDomains;
            this->currentMethodsHandlers = currentMethodsHandlers;

            this->defaultLoginRPCClient = currentLoginRPCClient;
            this->defaultAuthDomains = currentAuthDomains;
            this->defaultMethodsHandlers = currentMethodsHandlers;

            pingIntervalInSeconds = 20;
            keyDistFactor = 1.0;
            queuePushTimeoutInMS = 2000;
            maxMessageSize = 10*1024*1024;
            remoteExecutionTimeoutInMS = 5000;
            remoteExecutionDisconnectedTries = 10;
            rwTimeoutInSeconds = 40;
        }
        ~ParametersDefinitions()
        {
            if ( defaultLoginRPCClient != currentLoginRPCClient )
                delete currentLoginRPCClient;

            if ( defaultMethodsHandlers != currentMethodsHandlers )
                delete currentMethodsHandlers;

            if ( defaultAuthDomains != currentAuthDomains )
                delete currentAuthDomains;
        }
        /**
         * @brief pingIntvl Ping Interval (in seconds) - Set before connect / not thread safe.
         */
        uint32_t pingIntervalInSeconds;
        /**
         * @brief keyDistFactor float value from 0 to 1, 0 is no threads used, and 1 to allow in every thread. - Set before connect / not thread safe.
         */
        float keyDistFactor;
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
        uint32_t rwTimeoutInSeconds;
        /**
         * @brief currentLoginRPCClient Current login RPC client (in cbm)
         */
        Authentication::LoginRPCClientCBM * currentLoginRPCClient;
        /**
         * @brief currentMethodsHandlers current Methods Manager
         */
        API::MethodsHandler *currentMethodsHandlers;
        /**
         * @brief currentAuthDomains Current used domain authentication pool
         */
        Authentication::Domains * currentAuthDomains;
    private:
        Authentication::LoginRPCClientCBM * defaultLoginRPCClient;
        API::MethodsHandler *defaultMethodsHandlers;
        Authentication::Domains *defaultAuthDomains;
    };

    ////////////////////////////////
    /**
     * @brief FastRPC2 This class is designed to persist between connections...
     * @param appName the application name to be used on the authentication server.
     * @param threadsCount number of preloaded threads
     * @param taskQueues number of max queued tasks for each thread
     */
    FastRPC2( const std::string & appName = "",void *callbacksObj = nullptr, uint32_t threadsCount = 16, uint32_t taskQueues = 24);
    ~FastRPC2();
    /**
     * @brief stop Stop the thread pool.
     */
    void stop();

    /**
     * @brief connectionHandler Process Connection Stream and manage bidirectional events from each side (Q/A).
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param serverMode true if the remote peer is a server
     * @param remotePair remote pair IP address detected by the acceptor
     * @return
     */
    int connectionHandler(Mantids29::Network::Sockets::Socket_TLS * stream, bool remotePeerIsServer, const char * remotePair);


    json runRemoteLogin( const std::string &connectionKey,
                         const std::string & user,
                         const Authentication::Data & authData,
                         const std::string & domain = "",
                         json *error = nullptr );

    json runRemoteChangePassword( const std::string &connectionKey,
                         const Authentication::Data & oldAuthData,
                         const Authentication::Data & newAuthData,
                         json *error = nullptr );


    json runRemoteTestPassword( const std::string &connectionKey,
                         const Authentication::Data & authData,
                         json *error = nullptr );

    json runRemoteListPasswords( const std::string &connectionKey,
                         const Authentication::Data & authData,
                         json *error = nullptr );

    bool runRemoteLogout(  const std::string &connectionKey, json *error = nullptr  );


    // TODO: runRemoteRPCMethod for sending message to an specific connected user...
    // TODO: runRemoteRPCMethod for sending message to all connected users...
    // TODO: runRemoteRPCMethod for sending message to all connected users from specific group...
    // TODO: message persistence for non-connected users
    // TODO: list current sessions
    /**
     * @brief runRemoteRPCMethod Run Remote RPC Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @param methodName Method Name
     * @param payload Function Payload
     * @param error Error Return
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    json runRemoteRPCMethod( const std::string &connectionKey, const std::string &methodName, const json &payload , json * error, bool retryIfDisconnected = true, bool passSessionCommands = false );
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // For Internal use only:
    /**
     * @brief sendPings Internal function to send pings to every connected peer
     */
    void sendPings();
    /**
     * @brief waitPingInterval Wait interval for pings
     * @return true if ping interval completed, false if a signal closed the wait interval (eg. FastRPC2 destroyed)
     */
    bool waitPingInterval();


    /**
     * @brief callbacks This is where you define the callbacks before using this class...
     */
    CallbackDefinitions callbacks;
    /**
     * @brief parameters All configuration Parameters...
     */
    ParametersDefinitions parameters;


private:
    static void executeRPCTask(void * taskData);
    static void executeRPCLogin(void * taskData);
    static void executeRPCLogout(void * taskData);
    static void executeRPCChangePassword(void * taskData);
    static void executeRPCTestPassword(void * taskData);
    static void executeRPCListPassword(void * taskData);
    static void sendRPCAnswer(FastRPC2::TaskParameters * parameters, const std::string & answer, uint8_t execution);

    int processAnswer(FastRPC2::Connection *connection);
    int processQuery(Network::Sockets::Socket_TLS *stream,
                     const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket, FastRPC2::SessionPTR *session
                     );

    Mantids29::Threads::Safe::Map<std::string> connectionsByKeyId;



    std::map<std::string,std::string> connectionKeyToLogin;
    std::mutex mtConnectionKeyToLogin;

    std::multimap<std::string,std::string> loginToConnectionKey;
    std::mutex mtLoginToConnectionKey;

    // Methods:
    // method name -> method.
    // ? TP or
    Mantids29::Threads::Pool::ThreadPool * threadPool;

    std::thread pinger;
  //  void * overwriteObject;

    std::atomic<bool> finished;
    std::mutex mtPing;
    std::condition_variable cvPing;

    Authentication::LoginRPCClientCBM defaultLoginRPCClient;
    Authentication::Domains defaultAuthDomain;
    API::MethodsHandler defaultMethodsHandlers;

};

}}}}
#endif // FastRPC2_H


