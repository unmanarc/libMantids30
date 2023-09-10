#pragma once

#include <Mantids29/Helpers/json.h>
#include <Mantids29/Auth/session.h>

#include <Mantids29/Auth/domains.h>
#include <Mantids29/API_Monolith/methodshandler.h>

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
        Network::Sockets::Socket_Stream_Base *streamBack = nullptr;
        uint32_t maxMessageSize=0;
        void * caller = nullptr;
        FastRPC2::SessionPTR * sessionHolder = nullptr;
        Mantids29::API::Monolith::MethodsHandler *currentMethodsHandlers;
        Authentication::Domains * currentAuthDomains = nullptr;
        Threads::Sync::Mutex_Shared * doneSharedMutex = nullptr;
        Threads::Sync::Mutex * socketMutex = nullptr;
        std::string methodName, ipAddr, cn, userId;
        json payload;
        uint64_t requestId = 0;
        void * callbacks = nullptr;
    };

    class Connection : public Mantids29::Threads::Safe::MapItem
    {
    public:
        Connection()
        {
            terminated = false;
        }

        void * callbacks = nullptr;

        // Socket
        Mantids29::Network::Sockets::Socket_Stream_Base * stream = nullptr;
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

        void (*CB_MethodExecution_RequiredAuthorizerNotProvided)( void * obj, TaskParameters * parameters ) = nullptr;
        void (*CB_MethodExecution_RequiredSessionNotProvided)( void * obj, TaskParameters * parameters ) = nullptr;
        void (*CB_MethodExecution_ValidatedTemporaryAuthFactor)(void * obj, TaskParameters * parameters,const uint32_t & passIdx, const Mantids29::Authentication::Reason & authReason) = nullptr;
        void (*CB_MethodExecution_FailedValidationOnTemporaryAuthFactor)(void * obj, TaskParameters * parameters,const uint32_t & passIdx, const Mantids29::Authentication::Reason & authReason) = nullptr;
        void (*CB_MethodExecution_Starting)(void * obj, TaskParameters * parameters,const json & payloadIn) = nullptr;
        void (*CB_MethodExecution_NotAuthorized)(void * obj, TaskParameters * parameters,const json & reasons) = nullptr;
        void (*CB_MethodExecution_MethodNotFound)(void * obj, TaskParameters * parameters) = nullptr;
        void (*CB_MethodExecution_DomainNotFound)(void * obj, TaskParameters * parameters) = nullptr;
        void (*CB_MethodExecution_ExecutedOK)(void * obj, TaskParameters * parameters, const double & elapsedMS,const json & payloadOut ) = nullptr;
        void (*CB_MethodExecution_UnknownError)(void * obj, TaskParameters * parameters ) = nullptr;

        void (*CB_Login_HalfAuthenticationRequireNextFactor)(void * obj, TaskParameters * parameters,const json & nextFactorResponse ) = nullptr;
        void (*CB_Login_LoggedIn)(void * obj, TaskParameters * parameters,const json & nextFactorResponse, const std::string & user, const std::string & domain ) = nullptr;
        void (*CB_Login_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain ) = nullptr;
        void (*CB_Login_AuthenticationFailed)(void * obj, TaskParameters * parameters, const std::string & user, const std::string & domain, const Mantids29::Authentication::Reason & authReason ) = nullptr;

        void (*CB_PasswordChange_RequestedOK)(void * obj, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx ) = nullptr;
        void (*CB_PasswordChange_RequestFailed)(void * obj, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx ) = nullptr;
        void (*CB_PasswordChange_BadCredentials)(void * obj, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx , const Mantids29::Authentication::Reason & authReason) = nullptr;
        void (*CB_PasswordChange_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain, const uint32_t & credIdx ) = nullptr;
        void (*CB_PasswordChange_ImpersonationFailed)(void * obj, TaskParameters * parameters, const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx ) = nullptr;

        void (*CB_PasswordValidation_OK)(void * obj, TaskParameters * parameters,  const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx ) = nullptr;
        void (*CB_PasswordValidation_Failed)(void * obj, TaskParameters * parameters,  const std::string & userCaller, const std::string & userCalled, const std::string & domain, const uint32_t & credIdx, const Mantids29::Authentication::Reason & authReason ) = nullptr;
        void (*CB_PasswordValidation_InvalidDomain)(void * obj, TaskParameters * parameters, const std::string & domain, const uint32_t & credIdx ) = nullptr;

        void (*CB_Protocol_UnexpectedAnswerReceived)(FastRPC2::Connection *connection, const std::string &answer) = nullptr;
        void (*CB_OutgoingTask_FailedExecutionOnDisconnectedPeer)(const std::string &connectionKey, const std::string &methodName, const json &payload) = nullptr;
        void (*CB_IncommingTask_DroppingOnFullQueue)(FastRPC2::TaskParameters * params) = nullptr;
        void (*CB_OutgoingTask_FailedExecutionTimedOut)(const std::string &connectionKey, const std::string &methodName, const json &payload) = nullptr;


        void * obj = nullptr;
        // Mantids29::Authentication::getReasonText(authReason) < - to obtain the auth reason.
    };

    class ParametersDefinitions {
    public:
        ParametersDefinitions(
                API::Monolith::MethodsHandler *currentMethodsHandlers,
                Authentication::Domains * currentAuthDomains
                )
        {
            this->currentAuthDomains = currentAuthDomains;
            this->currentMethodsHandlers = currentMethodsHandlers;

            this->defaultAuthDomains = currentAuthDomains;
            this->defaultMethodsHandlers = currentMethodsHandlers;

            maxMessageSize = 10*1024*1024;
            queuePushTimeoutInMS = 2000;
            remoteExecutionTimeoutInMS = 5000;
            remoteExecutionDisconnectedTries = 10;
        }
        ~ParametersDefinitions()
        {
            if ( defaultMethodsHandlers != currentMethodsHandlers )
                delete currentMethodsHandlers;

            if ( defaultAuthDomains != currentAuthDomains )
                delete currentAuthDomains;
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
         * @brief currentMethodsHandlers current Methods Manager
         */
        API::Monolith::MethodsHandler *currentMethodsHandlers;
        /**
         * @brief currentAuthDomains Current used domain authentication pool
         */
        Authentication::Domains * currentAuthDomains;
    private:
        API::Monolith::MethodsHandler *defaultMethodsHandlers;
        Authentication::Domains *defaultAuthDomains;
    };

    ////////////////////////////////
    /**
     * @brief FastRPC2 This class is designed to persist between connections...
     * @param appName the application name to be used on the authentication server.
     * @param threadsCount number of preloaded threads
     * @param taskQueues number of max queued tasks for each thread
     */
    FastRPC2(const std::string & appName = "", uint32_t threadsCount = 16, uint32_t taskQueues = 24);
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

    json runRemoteListPasswords(const std::string &connectionKey, json *error = nullptr );

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
    CallbackDefinitions m_callbacks;
    /**
     * @brief parameters All configuration Parameters...
     */
    ParametersDefinitions m_parameters;

    bool useCNAsServerKey() const;
    void setUseCNAsServerKey(bool newUseCNAsServerKey);

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

    Mantids29::Threads::Safe::Map<std::string> m_connectionsByKeyId;


    static json passwordPublicDataToJSON(const uint32_t &idx, const Authentication::Secret_PublicData &publicData);

    // TODO:
    std::map<std::string,std::string> m_connectionKeyToLogin;
    std::mutex m_connectionKeyToLoginMutex;
    std::multimap<std::string,std::string> m_loginToConnectionKey;
    std::mutex m_loginToConnectionKeyMutex;


    // Methods:
    // method name -> method.
    // ? TP or
    Mantids29::Threads::Pool::ThreadPool * m_threadPool;

    std::thread m_pingerThread;
  //  void * overwriteObject;

    std::atomic<bool> m_isFinished;
    std::mutex m_pingMutex;
    std::condition_variable m_pingCondition;

    Authentication::Domains m_defaultAuthDomain;
    Mantids29::API::Monolith::MethodsHandler m_defaultMethodsHandlers;
    bool m_useCNAsServerKey = false;
};

}}}}


