#include "fastrpc2.h"
#include "Mantids29/Auth/ds_auth_reason.h"

#include <Mantids29/API_Monolith/methodshandler.h>
#include <Mantids29/Auth/multi.h>
#include <Mantids29/Helpers/callbacks.h>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Helpers/random.h>
#include <Mantids29/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids29/Net_Sockets/socket_tls.h>
#include <Mantids29/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>
#include <memory>

using namespace Mantids29;
using namespace Network::Sockets;
using namespace Network::Protocols::FastRPC;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

// TODO: listar usuarios logeados, registrar usuarios logeados? inicio de sesion/fin de sesion...

void vrsyncRPCPingerThread(FastRPC2 *obj)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "fRPC2:Pinger");
#endif

    while (obj->waitPingInterval())
    {
        obj->sendPings(); // send pings to every registered client...
    }
}

FastRPC2::FastRPC2(const string &appName, uint32_t threadsCount, uint32_t taskQueues)
    : m_defaultMethodsHandlers(appName)
    , m_parameters(&m_defaultMethodsHandlers, &m_defaultAuthDomain)
{
    m_threadPool = new Threads::Pool::ThreadPool(threadsCount, taskQueues);

    m_isFinished = false;

    m_threadPool->start();
    m_pingerThread = thread(vrsyncRPCPingerThread, this);
}

FastRPC2::~FastRPC2()
{
    // Set pings to cease (the current one will continue)
    m_isFinished = true;

    // Notify that pings should stop now... This can take a while (while pings are cycled)...
    {
        unique_lock<mutex> lk(m_pingMutex);
        m_pingCondition.notify_all();
    }

    // Wait until the loop ends...
    m_pingerThread.join();

    delete m_threadPool;
}

void FastRPC2::stop()
{
    m_threadPool->stop();
}

void FastRPC2::sendPings()
{
    // This will create some traffic:
    auto keys = m_connectionsByKeyId.getKeys();
    for (const auto &i : keys)
    {
        // Avoid to ping more hosts during program finalization...
        if (m_isFinished)
            return;
        // Run unexistant remote function
        runRemoteRPCMethod(i, "_pingNotFound_", {}, nullptr, false);
    }
}

bool FastRPC2::waitPingInterval()
{
    unique_lock<mutex> lk(m_pingMutex);
    if (m_pingCondition.wait_for(lk, S(m_parameters.pingIntervalInSeconds)) == cv_status::timeout)
    {
        return true;
    }
    return false;
}

int FastRPC2::processAnswer(FastRPC2::Connection *connection)
{
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) connection->callbacks);

    uint32_t maxAlloc = m_parameters.maxMessageSize;
    uint64_t requestId;
    uint8_t executionStatus;
    char *payloadBytes;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId = connection->stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    ////////////////////////////////////////////////////////////
    // READ IF EXECUTED.
    executionStatus = connection->stream->readU<uint8_t>();

    // READ THE PAYLOAD...
    payloadBytes = connection->stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    if (true)
    {
        unique_lock<mutex> lk(connection->answersMutex);
        if (connection->pendingRequests.find(requestId) != connection->pendingRequests.end())
        {
            connection->executionStatus[requestId] = executionStatus;

            Helpers::JSONReader2 reader;
            bool parsingSuccessful = reader.parse(payloadBytes, connection->answers[requestId]);
            if (parsingSuccessful)
            {
                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->answersCondition.notify_all();
            }
            else
            {
                // if not able to answer, please remove from answers...
                // TODO: notify this malformed data...
                connection->answers.erase(requestId);
                connection->executionStatus.erase(requestId);
                connection->pendingRequests.erase(requestId);
            }
        }
        else
        {
            CALLBACK(callbacks->CB_Protocol_UnexpectedAnswerReceived)(connection, payloadBytes);
        }
    }

    delete[] payloadBytes;
    return 1;
}

int FastRPC2::processQuery(Socket_TLS *stream, const string &key, const float &priority, Threads::Sync::Mutex_Shared *mtDone, Threads::Sync::Mutex *mtSocket, FastRPC2::SessionPTR *sessionHolder)
{
    uint32_t maxAlloc = m_parameters.maxMessageSize;
    uint64_t requestId;
    char *payloadBytes;
    bool ok;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId = stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    // READ THE METHOD NAME.
    string methodName = stream->readStringEx<uint8_t>(&ok);
    if (!ok)
    {
        return -2;
    }
    // READ THE PAYLOAD...
    payloadBytes = stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    auto session = sessionHolder->get();

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Helpers::JSONReader2 reader;
    FastRPC2::TaskParameters *params = new FastRPC2::TaskParameters;
    params->sessionHolder = sessionHolder;
    params->currentMethodsHandlers = m_parameters.currentMethodsHandlers;
    params->currentAuthDomains = m_parameters.currentAuthDomains;
    params->requestId = requestId;
    params->methodName = methodName;
    params->ipAddr = stream->getRemotePairStr();
    params->cn = stream->getTLSPeerCN();
    params->doneSharedMutex = mtDone;
    params->socketMutex = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->maxMessageSize = m_parameters.maxMessageSize;
    params->callbacks = &m_callbacks;
    params->userId = session ? session->getAuthUser() : "";

    bool parsingSuccessful = reader.parse(payloadBytes, params->payload);
    delete[] payloadBytes;

    if (!parsingSuccessful)
    {
        // Bad Incomming JSON... Disconnect
        delete params;
        return -3;
    }
    else
    {
        params->doneSharedMutex->lockShared();

        void (*currentTask)(void *) = executeRPCTask;

        if (params->methodName == "SESSION.LOGIN")
            currentTask = executeRPCLogin;
        else if (params->methodName == "SESSION.LOGOUT")
            currentTask = executeRPCLogout;
        else if (params->methodName == "SESSION.CHPASSWD")
            currentTask = executeRPCChangePassword;
        else if (params->methodName == "SESSION.TSTPASSWD")
            currentTask = executeRPCTestPassword;
        else if (params->methodName == "SESSION.LISTPASSWD")
            currentTask = executeRPCListPassword;

        if (!m_threadPool->pushTask(currentTask, params, m_parameters.queuePushTimeoutInMS, priority, key))
        {
            // Can't push the task in the queue. Null answer.
            CALLBACK(m_callbacks.CB_IncommingTask_DroppingOnFullQueue)(params);
            sendRPCAnswer(params, "", 3);
            params->doneSharedMutex->unlockShared();
            delete params;
        }
    }
    return 1;
}

bool FastRPC2::useCNAsServerKey() const
{
    return m_useCNAsServerKey;
}

void FastRPC2::setUseCNAsServerKey(bool newUseCNAsServerKey)
{
    m_useCNAsServerKey = newUseCNAsServerKey;
}

json FastRPC2::runRemoteLogin(const std::string &connectionKey, const std::string &user, const Authentication::Data &authData, const std::string &domain, json *error)
{
    json jAuthData;
    jAuthData["user"] = user;
    jAuthData["domain"] = domain;
    jAuthData["authData"] = authData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.LOGIN", jAuthData, error, true, true);
}

json FastRPC2::runRemoteChangePassword(const std::string &connectionKey, const Authentication::Data &oldAuthData, const Authentication::Data &newAuthData, json *error)
{
    return runRemoteChangePassword(connectionKey, "", oldAuthData, newAuthData, error);
}

json FastRPC2::runRemoteTestPassword(const std::string &connectionKey, const Authentication::Data &authData, json *error)
{
    return runRemoteTestPassword(connectionKey,"",authData,error);
}

json FastRPC2::runRemoteChangePassword(const std::string &connectionKey, const string &user, const Authentication::Data &oldAuthData, const Authentication::Data &newAuthData, json *error)
{
    json jAuthData;

    if (!user.empty())
    {
        jAuthData["impersonate"] = true;
        jAuthData["user"] = user;
    }

    jAuthData["oldAuth"] = oldAuthData.toJson();
    jAuthData["newAuth"] = newAuthData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.CHPASSWD", jAuthData, error, true, true);
}

json FastRPC2::runRemoteTestPassword(const std::string &connectionKey, const string &user, const Authentication::Data &authData, json *error)
{
    json jAuthData;

    if (!user.empty())
    {
        jAuthData["impersonate"] = true;
        jAuthData["user"] = user;
    }

    jAuthData["auth"] = authData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.TSTPASSWD", jAuthData, error, true, true);
}

json FastRPC2::runRemoteListPasswords(const std::string &connectionKey, json *error)
{
    return runRemoteListPasswords(connectionKey, "", error);
}

json FastRPC2::runRemoteListPasswords(const std::string &connectionKey, const string &user, json *error)
{
    json jAuthData;

    if (!user.empty())
    {
        jAuthData["impersonate"] = true;
        jAuthData["user"] = user;
    }

    return runRemoteRPCMethod(connectionKey, "SESSION.LISTPASSWD", jAuthData, error, true, true);
}

bool FastRPC2::runRemoteLogout(const string &connectionKey, json *error)
{
    json x = runRemoteRPCMethod(connectionKey, "SESSION.LOGOUT", {}, error, true, true);
    return JSON_ASBOOL(x, "ok", false);
}

int FastRPC2::connectionHandler(Network::Sockets::Socket_TLS *stream, bool remotePeerIsServer, const char *remotePair)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "VSRPC:ProcStr");
#endif

    int ret = 1;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC2::Connection *connection = new FastRPC2::Connection;
    connection->callbacks = &m_callbacks;
    connection->socketMutex = &mtSocket;

    if (remotePeerIsServer && !m_useCNAsServerKey)
        connection->key = "SERVER";
    else
        connection->key = !remotePeerIsServer ? stream->getTLSPeerCN() + "?" + Helpers::Random::createRandomHexString(8) : stream->getTLSPeerCN();

    connection->stream = stream;

    // TODO: multiple connections from the same key?
    if (!m_connectionsByKeyId.addElement(connection->key, connection))
    {
        delete connection;
        return -2;
    }

    stream->setReadTimeout(m_parameters.rwTimeoutInSeconds);
    stream->setWriteTimeout(m_parameters.rwTimeoutInSeconds);

    FastRPC2::SessionPTR session;

    while (ret > 0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU<uint8_t>(&readOK))
        {
        case 'A':
            // Process Answer, incomming answer for query, report to the caller...
            ret = processAnswer(connection);
            break;
        case 'Q':
            // Process Query, incomming query...
            ret = processQuery(stream, connection->key, m_parameters.keyDistFactor, &mtDone, &mtSocket, &session);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK) // <- read timeout.
                ret = -101;
            else
                ret = 0;
            break;
        default:
            // Invalid protocol.
            ret = -100;
            break;
        }
        //        connection->lastReceivedData = time(nullptr);
    }

    // Wait until all task are done.
    mtDone.lock();
    mtDone.unlock();

    stream->shutdownSocket();

    connection->terminated = true;
    connection->answersCondition.notify_all();
    m_connectionsByKeyId.destroyElement(connection->key);

    return ret;
}

Authentication::Reason temporaryAuthentication(FastRPC2::TaskParameters *params, const std::string &userName, const std::string &domainName, const Authentication::Data &authData)
{
    Authentication::Reason eReason;

    // Current Auth Domains is already checked for existence in executeRPCTask:
    auto auth = params->currentAuthDomains->openDomain(domainName);
    if (!auth)
        eReason = Authentication::REASON_INVALID_DOMAIN;
    else
    {
        Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = params->ipAddr;
        clientDetails.tlsCommonName = params->cn;
        clientDetails.userAgent = "FastRPC2";

        eReason = auth->authenticate(params->currentMethodsHandlers->getApplicationName(),
                                     clientDetails,
                                     userName,
                                     authData.m_password,
                                     authData.m_passwordIndex); // Authenticate in a non-persistent fashion.
        params->currentAuthDomains->releaseDomain(domainName);
    }

    return eReason;
}

void FastRPC2::executeRPCTask(void *vTaskParams)
{
    TaskParameters *taskParams = (TaskParameters *) (vTaskParams);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);
    std::shared_ptr<Authentication::Session> session = taskParams->sessionHolder->get();

    json response;
    json rsp;
    response["ret"] = 0;

    Helpers::JSONReader2 reader;

    std::string userName = JSON_ASSTRING(taskParams->payload["extraAuth"], "user", "");
    std::string domainName = JSON_ASSTRING(taskParams->payload["extraAuth"], "domain", "");

    Authentication::Multi extraAuths;
    extraAuths.setJson(taskParams->payload["extraAuth"]["data"]);

    // If there is a session, overwrite the user/domain inputs...
    if (session)
    {
        userName = session->getAuthUser();
        domainName = session->getAuthenticatedDomain();
    }

    if (!taskParams->currentAuthDomains)
    {
        CALLBACK(callbacks->CB_MethodExecution_RequiredAuthorizerNotProvided)(callbacks->obj, taskParams);
        return;
    }

    if (taskParams->currentMethodsHandlers->getMethodRequireFullAuth(taskParams->methodName) && !session)
    {
        CALLBACK(callbacks->CB_MethodExecution_RequiredSessionNotProvided)(callbacks->obj, taskParams);
        return;
    }

    // TODO: what happens if we are given with unhandled but valid auths that should not be validated...?
    // Get/Pass the temporary authentications for null and not-null sessions:
    std::set<uint32_t> extraTmpIndexes;
    for (const uint32_t &passIdx : extraAuths.getAvailableIndices())
    {
        Authentication::Reason authReason = temporaryAuthentication(taskParams, userName, domainName, extraAuths.getAuthentication(passIdx));

        // Include the pass idx in the Extra TMP Index.
        if (Authentication::IS_PASSWORD_AUTHENTICATED(authReason))
        {
            CALLBACK(callbacks->CB_MethodExecution_ValidatedTemporaryAuthFactor)(callbacks->obj, taskParams, passIdx, authReason);
            extraTmpIndexes.insert(passIdx);
        }
        else
        {
            CALLBACK(callbacks->CB_MethodExecution_FailedValidationOnTemporaryAuthFactor)(callbacks->obj, taskParams, passIdx, authReason);
        }
    }

    bool found = false;

    auto authorizer = taskParams->currentAuthDomains->openDomain(domainName);
    if (authorizer)
    {
        json reasons;

        // Validate that the RPC method is ready to go (fully authorized and no password is expired).
        auto i = taskParams->currentMethodsHandlers->validatePermissions(authorizer, session.get(), taskParams->methodName, extraTmpIndexes, &reasons);

        taskParams->currentAuthDomains->releaseDomain(domainName);

        switch (i)
        {
        case API::Monolith::MethodsHandler::VALIDATION_OK:
        {
            if (session)
                session->updateLastActivity();

            // Report:
            CALLBACK(callbacks->CB_MethodExecution_Starting)(callbacks->obj, taskParams, taskParams->payload);

            auto start = chrono::high_resolution_clock::now();
            auto finish = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = finish - start;

            switch (taskParams->currentMethodsHandlers->invoke(taskParams->currentAuthDomains, domainName, session.get(), taskParams->methodName, taskParams->payload, &rsp))
            {
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

                finish = chrono::high_resolution_clock::now();
                elapsed = finish - start;

                CALLBACK(callbacks->CB_MethodExecution_ExecutedOK)(callbacks->obj, taskParams, elapsed.count(), rsp);

                found = true;
                response["ret"] = 200;
                break;
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:

                CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
                response["ret"] = 404;
                break;
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_INVALIDDOMAIN:
                // This code should never be executed... <
                CALLBACK(callbacks->CB_MethodExecution_DomainNotFound)(callbacks->obj, taskParams);
                response["ret"] = 404;
                break;
            default:
                CALLBACK(callbacks->CB_MethodExecution_UnknownError)(callbacks->obj, taskParams);
                response["ret"] = 401;
                break;
            }
        }
        break;
        case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            CALLBACK(callbacks->CB_MethodExecution_NotAuthorized)(callbacks->obj, taskParams, reasons);
            response["auth"]["reasons"] = reasons;
            response["ret"] = 401;
        }
        break;
        case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
        default:
        {
            CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
            response["ret"] = 404;
        }
        break;
        }
    }
    else
    {
        CALLBACK(callbacks->CB_MethodExecution_DomainNotFound)(callbacks->obj, taskParams);
        // Domain Not found.
        response["ret"] = 404;
    }

    //Json::StreamWriterBuilder builder;
    //builder.settings_["indentation"] = "";

    //
    response["payload"] = rsp;
    sendRPCAnswer(taskParams, response.toStyledString(), found ? 2 : 4);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC2::executeRPCLogin(void *taskData)
{
    FastRPC2::TaskParameters *taskParams = (FastRPC2::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    // CREATE NEW SESSION:
    json response;
    Authentication::Reason authReason = Authentication::REASON_INTERNAL_ERROR;

    auto session = taskParams->sessionHolder->get();

    std::string user = JSON_ASSTRING(taskParams->payload, "user", "");
    std::string domain = JSON_ASSTRING(taskParams->payload, "domain", "");
    Authentication::Data authData;
    authData.setJson(taskParams->payload["authData"]);
    std::map<uint32_t, std::string> stAccountPassIndexesUsedForLogin;

    if (session == nullptr && authData.m_passwordIndex != 0)
    {
        // Why are you trying to authenticate this way?
        response["txt"] = getReasonText(authReason);
        response["val"] = static_cast<Json::UInt>(authReason);
        response["nextPassReq"] = false;
    }
    else if (!taskParams->currentAuthDomains)
    {
        // You should not be trying to authenticate against me...
        authReason = Authentication::REASON_NOT_IMPLEMENTED;
        response["txt"] = getReasonText(authReason);
        response["val"] = static_cast<Json::UInt>(authReason);
        response["nextPassReq"] = false;
    }
    else
    {
        // PROCEED THEN....

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(domain);
        if (domainAuthenticator)
        {
            Authentication::ClientDetails clientDetails;
            clientDetails.ipAddress = taskParams->ipAddr;
            clientDetails.tlsCommonName = taskParams->cn;
            clientDetails.userAgent = "FastRPC2 AGENT";

            authReason = domainAuthenticator->authenticate(taskParams->currentMethodsHandlers->getApplicationName(),
                                                           clientDetails,
                                                           user,
                                                           authData.m_password,
                                                           authData.m_passwordIndex,
                                                           Authentication::MODE_PLAIN,
                                                           "",
                                                           &stAccountPassIndexesUsedForLogin);
            taskParams->currentAuthDomains->releaseDomain(domain);
        }
        else
        {
            CALLBACK(callbacks->CB_Login_InvalidDomain)(callbacks->obj, taskParams, domain);
            authReason = Authentication::REASON_INVALID_DOMAIN;
        }

        if (Authentication::IS_PASSWORD_AUTHENTICATED(authReason))
        {
            // If not exist an authenticated session, create a new one.
            if (!session)
            {
                session = taskParams->sessionHolder->create(taskParams->currentMethodsHandlers->getApplicationName());
                if (session)
                {
                    session->setPersistentSession(true);
                    session->registerPersistentAuthentication(user, domain, authData.m_passwordIndex, authReason);

                    // The first pass/time the list of idx should be filled into.
                    if (authData.m_passwordIndex == 0)
                        session->setRequiredBasicAuthenticationIndices(stAccountPassIndexesUsedForLogin);
                }
            }
            else
            {
                // If exist, just register the current authentication into that session and return the current sessionid
                session->registerPersistentAuthentication(user, domain, authData.m_passwordIndex, authReason);
            }
        }
        else
        {
            CALLBACK(callbacks->CB_Login_AuthenticationFailed)(callbacks->obj, taskParams, user, domain, authReason);
        }

        response["txt"] = getReasonText(authReason);
        response["val"] = static_cast<Json::UInt>(authReason);
        response["nextPassReq"] = false;

        if (session)
        {
            auto i = session->getNextRequiredAuthenticationIndex();
            if (i.first != 0xFFFFFFFF)
            {
                // No next login idx.
                response.removeMember("nextPassReq");
                response["nextPassReq"]["idx"] = i.first;
                response["nextPassReq"]["desc"] = i.second;
                CALLBACK(callbacks->CB_Login_HalfAuthenticationRequireNextFactor)(callbacks->obj, taskParams, response);
            }
            else
            {
                CALLBACK(callbacks->CB_Login_LoggedIn)(callbacks->obj, taskParams, response, user, domain);
            }
        }
        else
        {
            // Not logged in...
        }
    }

    sendRPCAnswer(taskParams, response.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC2::executeRPCLogout(void *taskData)
{
    FastRPC2::TaskParameters *params = (FastRPC2::TaskParameters *) (taskData);
    json response;

    response["ok"] = params->sessionHolder->destroy();

    sendRPCAnswer(params, response.toStyledString(), 2);
    params->doneSharedMutex->unlockShared();
}

void FastRPC2::executeRPCChangePassword(void *taskData)
{
    FastRPC2::TaskParameters *taskParams = (FastRPC2::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    json response;
    response["ok"] = false;

    auto session = taskParams->sessionHolder->get();
    Authentication::Data newAuth, oldAuth;

    if (!taskParams->currentAuthDomains)
    {
        response["reason"] = "NOT_AUTHENTICABLE";
    }
    else if (!session || !session->isFullyAuthenticated(Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        // TODO: what if expired?, here we admit expired password to pass...
        response["reason"] = "BAD_SESSION";
    }
    else if (!newAuth.setJson(taskParams->payload["newAuth"]))
    {
        // Error parsing credentials...
        response["reason"] = "ERROR_PARSING_AUTH";
    }
    else if (!oldAuth.setJson(taskParams->payload["oldAuth"]))
    {
        // Error parsing credentials...
        response["reason"] = "ERROR_PARSING_AUTH";
    }
    else if (oldAuth.m_passwordIndex != newAuth.m_passwordIndex)
    {
        response["reason"] = "IDX_DIFFER";
    }
    else
    {
        uint32_t credIdx = newAuth.m_passwordIndex;

        auto userCaller = session->getAuthUser();
        auto userCalled = session->getAuthUser();
        auto authDomain = session->getAuthenticatedDomain();

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(authDomain);
        if (domainAuthenticator)
        {
            bool ok = true, impersonated = false;

            if (JSON_ASBOOL(taskParams->payload, "impersonate", false) == true)
            {
                userCalled = JSON_ASSTRING(taskParams->payload, "user", userCaller);

                if (domainAuthenticator->isAccountSuperUser(userCaller))
                {
                    ok = true;
                    impersonated = true;
                }
                else
                {
                    ok = false;

                    response["reason"] = "ERROR_IMPERSONATION_FAILED";
                    CALLBACK(callbacks->CB_ImpersonationFailed)
                    (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx);
                }
            }

            if (ok)
            {
                Authentication::ClientDetails clientDetails;
                clientDetails.ipAddress = taskParams->ipAddr;
                clientDetails.tlsCommonName = taskParams->cn;
                clientDetails.userAgent = "FastRPC2 AGENT";

                Authentication::Reason authReason;

                if (impersonated)
                    authReason = Authentication::REASON_AUTHENTICATED;
                else
                    authReason = domainAuthenticator->authenticate(taskParams->currentMethodsHandlers->getApplicationName(), clientDetails, userCalled, oldAuth.m_password, credIdx);

                if (IS_PASSWORD_AUTHENTICATED(authReason))
                {
                    // TODO: alternative/configurable password storage...
                    // TODO: check password policy.
                    Authentication::Secret newSecretData = Authentication::createNewSecret(newAuth.m_password, Authentication::FN_SSHA256);

                    if (impersonated)
                        response["ok"] = domainAuthenticator->accountChangeSecret(userCalled, newSecretData, credIdx);
                    else
                        response["ok"] = domainAuthenticator->accountChangeAuthenticatedSecret(taskParams->currentMethodsHandlers->getApplicationName(),
                                                                                                userCalled,
                                                                                                credIdx,
                                                                                                oldAuth.m_password,
                                                                                                newSecretData,
                                                                                                clientDetails);

                    if (JSON_ASBOOL(response, "ok", false) == true)
                    {
                        response["reason"] = "OK";
                        CALLBACK(callbacks->CB_PasswordChange_RequestedOK)
                        (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx);
                    }
                    else
                    {
                        response["reason"] = "ERROR_CHANGING_PASSWORD";
                        CALLBACK(callbacks->CB_PasswordChange_RequestFailed)
                        (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx);
                    }
                }
                else
                {
                    CALLBACK(callbacks->CB_PasswordChange_BadCredentials)
                    (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx, authReason);

                    response["reason"] = "INVALID_PASSWORD";
                    response["accountDisabledStatus"] = false;
                    if (domainAuthenticator->isAccountDisabled(userCalled))
                    {
                        response["accountDisabledStatus"] = true;

                        // SESSION IS deauthed, so no execution will be allowed after this (only if the account is disabled)...
                        session = nullptr;
                        taskParams->sessionHolder->destroy();
                    }
                }

                Authentication::Secret_PublicData publicData = domainAuthenticator->getAccountSecretPublicData(userCalled, credIdx);
                response["credPublicData"] = passwordPublicDataToJSON(credIdx, publicData);
            }

            taskParams->currentAuthDomains->releaseDomain(authDomain);
        }
        else
        {
            response["reason"] = "DOMAIN_NOT_FOUND";
            CALLBACK(callbacks->CB_PasswordChange_InvalidDomain)(callbacks->obj, taskParams, authDomain, credIdx);
        }
    }

    sendRPCAnswer(taskParams, response.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC2::executeRPCTestPassword(void *taskData)
{
    FastRPC2::TaskParameters *taskParams = (FastRPC2::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    Authentication::Data auth;
    json response;
    auto session = taskParams->sessionHolder->get();
    response["ok"] = false;

    // TODO:
    if (!taskParams->currentAuthDomains)
    {
        response["reason"] = "NOT_AUTHENTICABLE";
    }
    else if (!session || !session->isFullyAuthenticated(Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        // TODO: what if expired?, here we admit expired password to pass...
        response["reason"] = "BAD_SESSION";
    }
    else if (!auth.setJson(taskParams->payload["auth"]))
    {
        // Error parsing credentials...
        response["reason"] = "ERROR_PARSING_AUTH";
    }
    else
    {
        uint32_t credIdx = auth.m_passwordIndex;

        auto userCaller = session->getAuthUser();
        auto userCalled = session->getAuthUser();
        auto authDomain = session->getAuthenticatedDomain();
        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(authDomain);

        if (domainAuthenticator)
        {
            bool ok = true;

            if (JSON_ASBOOL(taskParams->payload, "impersonate", false) == true)
            {
                userCalled = JSON_ASSTRING(taskParams->payload, "user", userCaller);

                if (domainAuthenticator->isAccountSuperUser(userCaller))
                {
                    ok = true;
                }
                else
                {
                    ok = false;

                    response["reason"] = "ERROR_IMPERSONATION_FAILED";
                    CALLBACK(callbacks->CB_ImpersonationFailed)
                    (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx);
                }
            }

            if (ok)
            {
                Authentication::ClientDetails clientDetails;
                clientDetails.ipAddress = taskParams->ipAddr;
                clientDetails.tlsCommonName = taskParams->cn;
                clientDetails.userAgent = "FastRPC2 AGENT";

                auto authReason = domainAuthenticator->authenticate(taskParams->currentMethodsHandlers->getApplicationName(), clientDetails, userCalled, auth.m_password, credIdx);
                if (IS_PASSWORD_AUTHENTICATED(authReason))
                {
                    response["ok"] = true;
                    CALLBACK(callbacks->CB_PasswordValidation_OK)
                    (callbacks->obj, taskParams, userCaller, userCalled, authDomain, credIdx);
                }
                else
                {
                    // DEAUTH:
                    response["reason"] = "INVALID_PASSWORD";

                    response["accountDisabledStatus"] = false;
                    if (domainAuthenticator->isAccountDisabled(userCalled))
                    {
                        response["accountDisabledStatus"] = true;

                        // SESSION IS deauthed, so no execution will be allowed after this (only if the account is disabled)...
                        session = nullptr;
                        taskParams->sessionHolder->destroy();
                    }

                    CALLBACK(callbacks->CB_PasswordValidation_Failed)
                    (callbacks->obj, taskParams,userCaller, userCalled, authDomain, credIdx, authReason);
                }

                Authentication::Secret_PublicData publicData = domainAuthenticator->getAccountSecretPublicData(userCalled, credIdx);
                response["credPublicData"] = passwordPublicDataToJSON(credIdx, publicData);
            }
            taskParams->currentAuthDomains->releaseDomain(authDomain);
        }
        else
        {
            response["reason"] = "BAD_DOMAIN";
            CALLBACK(callbacks->CB_PasswordValidation_InvalidDomain)(callbacks->obj, taskParams, authDomain, credIdx);
        }
    }

    sendRPCAnswer(taskParams, response.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

json FastRPC2::passwordPublicDataToJSON(const uint32_t &idx, const Authentication::Secret_PublicData &publicData)
{
    json r;
    r["badAtttempts"] = publicData.badAttempts;
    r["forceExpiration"] = publicData.forceExpiration;
    r["nul"] = publicData.nul;
    r["passwordFunction"] = publicData.passwordFunction;
    r["expiration"] = (Json::UInt64) publicData.expiration;
    r["description"] = publicData.description;
    r["isExpired"] = publicData.isExpired();
    r["isRequiredAtLogin"] = publicData.requiredAtLogin;
    r["isLocked"] = publicData.locked;
    r["idx"] = idx;
    return r;
}

void FastRPC2::executeRPCListPassword(void *taskData)
{
    FastRPC2::TaskParameters *taskParams = (FastRPC2::TaskParameters *) (taskData);
    CallbackDefinitions *callbacks = ((CallbackDefinitions *) taskParams->callbacks);

    json response;
    response["ok"] = false;
    auto session = taskParams->sessionHolder->get();

    if (!taskParams->currentAuthDomains)
    {
        response["reason"] = "NOT_AUTHENTICABLE";
    }
    else if (!session || !session->isFullyAuthenticated(Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        response["reason"] = "BAD_SESSION";
    }
    else
    {

        auto userCaller = session->getAuthUser();
        auto userCalled = session->getAuthUser();
        auto authDomain = session->getAuthenticatedDomain();

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(authDomain);
        if (domainAuthenticator)
        {
            bool ok = true;
            if (JSON_ASBOOL(taskParams->payload, "impersonate", false) == true)
            {
                userCalled = JSON_ASSTRING(taskParams->payload, "user", userCaller);

                if (domainAuthenticator->isAccountSuperUser(userCaller))
                {
                    ok = true;
                }
                else
                {
                    ok = false;

                    response["reason"] = "ERROR_IMPERSONATION_FAILED";
                    CALLBACK(callbacks->CB_ImpersonationFailed)
                    (callbacks->obj, taskParams, userCaller, userCalled, authDomain, 0);
                }
            }

            if (ok)
            {
                std::map<uint32_t, Authentication::Secret_PublicData> publics = domainAuthenticator->getAccountAllSecretsPublicData(userCalled);
                response["ok"] = true;

                uint32_t ix = 0;
                for (const auto &i : publics)
                {
                    response["list"][ix] = passwordPublicDataToJSON(i.first, i.second);
                    ix++;
                }
            }

            taskParams->currentAuthDomains->releaseDomain(authDomain);
        }
        else
            response["reason"] = "BAD_DOMAIN";
    }

    sendRPCAnswer(taskParams, response.toStyledString(), 2);
    taskParams->doneSharedMutex->unlockShared();
}

void FastRPC2::sendRPCAnswer(FastRPC2::TaskParameters *params, const string &answer, uint8_t execution)
{
    // Send a block.
    params->socketMutex->lock();
    if (params->streamBack->writeU<uint8_t>('A') && // ANSWER
        params->streamBack->writeU<uint64_t>(params->requestId) && params->streamBack->writeU<uint8_t>(execution)
        && params->streamBack->writeStringEx<uint32_t>(answer.size() <= params->maxMessageSize ? answer : "", params->maxMessageSize))
    {
    }
    params->socketMutex->unlock();
}

json FastRPC2::runRemoteRPCMethod(const string &connectionKey, const string &methodName, const json &payload, json *error, bool retryIfDisconnected, bool passSessionCommands)
{
    json r;

    if (!passSessionCommands && boost::starts_with(methodName, "SESSION."))
        return r;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    string output = Json::writeString(builder, payload);

    if (output.size() > m_parameters.maxMessageSize)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 1;
            (*error)["errorMessage"] = "Payload exceed the Maximum Message Size.";
        }
        return r;
    }

    FastRPC2::Connection *connection;

    uint32_t _tries = 0;
    while ((connection = (FastRPC2::Connection *) m_connectionsByKeyId.openElement(connectionKey)) == nullptr)
    {
        _tries++;
        if (_tries >= m_parameters.remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            CALLBACK(m_callbacks.CB_OutgoingTask_FailedExecutionOnDisconnectedPeer)(connectionKey, methodName, payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 2;
                (*error)["errorMessage"] = "Abort after remote peer not found/connected.";
            }
            return r;
        }
        sleep(1);
    }

    uint64_t requestId;
    // Create a request ID.
    connection->mtReqIdCt.lock();
    requestId = connection->requestIdCounter++;
    connection->mtReqIdCt.unlock();

    if (1)
    {
        unique_lock<mutex> lk(connection->answersMutex);
        // Create authorization to be inserted:
        connection->pendingRequests.insert(requestId);
    }

    connection->socketMutex->lock();
    if (connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
        connection->stream->writeU<uint64_t>(requestId) && connection->stream->writeStringEx<uint8_t>(methodName) && connection->stream->writeStringEx<uint32_t>(output, m_parameters.maxMessageSize))
    {
    }
    connection->socketMutex->unlock();

    // Time to wait for answers...
    for (;;)
    {
        unique_lock<mutex> lk(connection->answersMutex);

        // Process multiple signals until our answer comes...

        if (connection->answersCondition.wait_for(lk, Ms(m_parameters.remoteExecutionTimeoutInMS)) == cv_status::timeout)
        {
            // break by timeout. (no answer)
            CALLBACK(m_callbacks.CB_OutgoingTask_FailedExecutionTimedOut)(connectionKey, methodName, payload);

            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 3;
                (*error)["errorMessage"] = "Remote Execution Timed Out: No Answer Received.";
            }
            break;
        }

        if (lk.owns_lock() && connection->answers.find(requestId) != connection->answers.end())
        {
            // break by element found. (answer)
            uint8_t executionStatus = connection->executionStatus[requestId];
            r = connection->answers[requestId];
            if (error)
            {
                switch (executionStatus)
                {
                case 2:
                    (*error)["succeed"] = true;
                    (*error)["errorId"] = 0;
                    (*error)["errorMessage"] = "Execution OK.";
                    break;
                case 3:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 4;
                    (*error)["errorMessage"] = "Remote Execution Failed: Full Queue.";
                    break;
                case 4:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 5;
                    (*error)["errorMessage"] = "Remote Execution Failed: Method Not Found.";
                    break;
                default:
                    (*error)["succeed"] = false;
                }
            }
            break;
        }

        if (lk.owns_lock() && connection->terminated)
        {
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 6;
                (*error)["errorMessage"] = "Connection is terminated: No Answer Received.";
            }
            break;
        }
    }

    if (1)
    {
        unique_lock<mutex> lk(connection->answersMutex);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    m_connectionsByKeyId.releaseElement(connectionKey);

    if (error)
    {
        if (!error->isMember("succeed"))
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 99;
            (*error)["errorMessage"] = "Unknown Error.";
        }
    }

    return r;
}

bool FastRPC2::runRemoteClose(const string &connectionKey)
{
    bool r = false;

    FastRPC2::Connection *connection;
    if ((connection = (FastRPC2::Connection *) m_connectionsByKeyId.openElement(connectionKey)) != nullptr)
    {
        connection->socketMutex->lock();
        if (connection->stream->writeU<uint8_t>(0))
        {
        }
        connection->socketMutex->unlock();

        m_connectionsByKeyId.releaseElement(connectionKey);
    }
    else
    {
        CALLBACK(m_callbacks.CB_OutgoingTask_FailedExecutionOnDisconnectedPeer)(connectionKey, "CLOSE", {});
    }
    return r;
}

set<string> FastRPC2::getConnectionKeys()
{
    return m_connectionsByKeyId.getKeys();
}

bool FastRPC2::checkConnectionKey(const string &connectionKey)
{
    return m_connectionsByKeyId.isMember(connectionKey);
}
