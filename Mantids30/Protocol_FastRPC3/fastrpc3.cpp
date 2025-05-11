#include "fastrpc3.h"
#include "Mantids30/Net_Sockets/socket_stream.h"
#include <Mantids30/API_Monolith/methodshandler.h>
#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>
#include <memory>

using namespace Mantids30;
using namespace Network::Sockets;
using namespace Network::Protocols::FastRPC;
using namespace Mantids30::DataFormat;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

// TODO: listar usuarios logeados en la app, registrar usuarios logeados? inicio de sesion/fin de sesion...

void vrsyncRPCPingerThread(FastRPC3 *obj)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "fRPC3:Pinger");
#endif

    while (obj->waitPingInterval())
    {
        obj->pingAllActiveConnections(); // send pings to every registered client...
    }
}

FastRPC3::FastRPC3(std::shared_ptr<DataFormat::JWT> jwtValidator, uint32_t threadsCount, uint32_t taskQueues)
    : m_defaultMethodsHandlers()
    , config(jwtValidator)
{
    m_defaultMethodsHandlers = std::make_shared<Mantids30::API::Monolith::MethodsHandler>();
    config.setDefaultHandlers( m_defaultMethodsHandlers );

    m_threadPool = new Threads::Pool::ThreadPool(threadsCount, taskQueues);
    m_isFinished = false;
    m_threadPool->start();

    m_pingerThread = thread(vrsyncRPCPingerThread, this);
}

FastRPC3::FastRPC3(uint32_t threadsCount, uint32_t taskQueues)
    : m_defaultMethodsHandlers()
    , config(std::make_shared<DataFormat::JWT>())
{
    m_defaultMethodsHandlers = std::make_shared<Mantids30::API::Monolith::MethodsHandler>();
    config.setDefaultHandlers( m_defaultMethodsHandlers );

    m_threadPool = new Threads::Pool::ThreadPool(threadsCount, taskQueues);
    m_isFinished = false;
    m_threadPool->start();

    m_pingerThread = thread(vrsyncRPCPingerThread, this);
}

FastRPC3::~FastRPC3()
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

void FastRPC3::stop()
{
    m_threadPool->stop();
}

void FastRPC3::pingAllActiveConnections()
{
    // This will create some traffic:
    auto keys = m_connectionMapById.getKeys();
    for (const auto &i : keys)
    {
        // Avoid to ping more hosts during program finalization...
        if (m_isFinished)
            return;
        // Run unexistant remote function to update the las received message.
        remote(i).executeTask("_pingNotFound_", {}, nullptr, false);
    }
}

bool FastRPC3::waitPingInterval()
{
    unique_lock<mutex> lk(m_pingMutex);
    if (m_pingCondition.wait_for(lk, S(config.pingIntervalInSeconds)) == cv_status::timeout)
    {
        return true;
    }
    return false;
}

int FastRPC3::processIncomingAnswer(FastRPC3::Connection *connection)
{
    RPC3CallbackDefinitions *callbacks = ((RPC3CallbackDefinitions *) connection->callbacks);

    uint32_t maxAlloc = config.maxMessageSize;
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
            CALLBACK(callbacks->onProtocolUnexpectedResponse)(connection, payloadBytes);
        }
    }

    delete[] payloadBytes;
    return 1;
}

int FastRPC3::processIncomingExecutionRequest(std::shared_ptr<Socket_Stream> stream, const string &key, const float &priority, Threads::Sync::Mutex_Shared *mtDone, Threads::Sync::Mutex *mtSocket, FastRPC3::SessionPTR *sessionHolder)
{
    uint32_t maxAlloc = config.maxMessageSize;
    uint64_t requestId = 0;
    uint8_t flags = 0;
    char *payloadBytes = nullptr, *extraAuthToken = nullptr;
    bool ok = false;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId = stream->readU<uint64_t>();
    if (!requestId)
    {
        return CONNECTION_FAILED_READING_REQUESTID;
    }

    flags = stream->readU<uint8_t>();
    if (flags==0)
    {
        return CONNECTION_FAILED_READING_FLAGS;
    }

    // READ THE METHOD NAME.
    string methodName = stream->readStringEx<uint8_t>(&ok);
    if (!ok)
    {
        return CONNECTION_FAILED_READING_METHOD_NAME;
    }
    // READ THE PAYLOAD...
    payloadBytes = stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return CONNECTION_FAILED_READING_PAYLOAD;
    }

    if ((flags&EXEC_FLAG_EXTRAAUTH) != 0)
    {
        extraAuthToken = stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
        if (!extraAuthToken)
        {
            delete[] payloadBytes;
            return CONNECTION_FAILED_READING_EXTRAAUTH;
        }
    }
    
    std::shared_ptr<Sessions::Session> session = sessionHolder->getSharedPointer();

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Helpers::JSONReader2 reader;
    std::shared_ptr<FastRPC3::TaskParameters> params = std::make_shared<FastRPC3::TaskParameters>();
    params->sessionHolder = sessionHolder;
    params->methodsHandler = config.methodHandlers;
    params->jwtValidator = config.jwtValidator;
    params->requestId = requestId;
    params->methodName = methodName;
    params->extraTokenAuth = extraAuthToken;
    params->remotePeerIPAddress = stream->getRemotePairStr();
    params->remotePeerTLSCommonName = stream->getPeerName();
    params->doneSharedMutex = mtDone;
    params->socketMutex = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->maxMessageSize = config.maxMessageSize;
    params->callbacks = &rpcCallbacks;
    params->userId = session ? session->getUser() : "";
    params->domain = session ? session->getDomain() : "";

    bool parsingSuccessful = reader.parse(payloadBytes, params->payload);
    delete[] payloadBytes;

    if (!parsingSuccessful)
    {
        // Bad Incoming JSON... Disconnect
        return CONNECTION_FAILED_PARSING_PAYLOAD;
    }
    else
    {
        params->doneSharedMutex->lockShared();

        void (*currentTask)(std::shared_ptr<void>) = LocalRPCTasks::executeLocalTask;

        if (params->methodName == "SESSION.LOGIN")
            currentTask = LocalRPCTasks::login;
        else if (params->methodName == "SESSION.LOGOUT")
            currentTask =  LocalRPCTasks::logout;
        else if (params->methodName == "SESSION.GETSSODATA")
            currentTask =  LocalRPCTasks::getSSOData;

        if (!m_threadPool->pushTask(currentTask, params, config.queuePushTimeoutInMS, priority, key))
        {
            // Can't push the task in the queue. Null answer.
            CALLBACK(rpcCallbacks.onIncomingTaskDroppedQueueFull)(params.get());
            sendRPCAnswer(params.get(), "", EXEC_STATUS_ERR_REMOTE_QUEUE_OVERFLOW);
            params->doneSharedMutex->unlockShared();
//            delete params;
        }
    }
    return CONNECTION_CONTINUE;
}

bool FastRPC3::isUsingRemotePeerCommonNameAsConnectionId() const
{
    return m_usingRemotePeerCommonNameAsConnectionId;
}

void FastRPC3::setUsingRemotePeerCommonNameAsConnectionId(const bool &newUseCNAsServerKey)
{
    m_usingRemotePeerCommonNameAsConnectionId = newUseCNAsServerKey;
}

int FastRPC3::handleConnection(std::shared_ptr<Sockets::Socket_Stream> stream, bool remotePeerIsServer)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "VSRPC:ProcStr");
#endif

    int ret = 1;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC3::Connection *connection = new FastRPC3::Connection;
    connection->callbacks = &rpcCallbacks;
    connection->socketMutex = &mtSocket;

    if (remotePeerIsServer)
    {
        if (m_usingRemotePeerCommonNameAsConnectionId)
        {
            connection->key = stream->getPeerName();
        }
        else
        {
            connection->key = "SERVER";
        }
    }
    else
    {
        connection->key = stream->getConnectionName();
    }

    /*
    if (remotePeerIsServer && !m_usingRemotePeerCommonNameAsConnectionId)
    {
        connection->key = "SERVER";
    }
    else
    {
        connection->key = !remotePeerIsServer ? stream->getPeerName() + "?" + Helpers::Random::createRandomHexString(8) : stream->getPeerName();
    }*/

    connection->stream = stream;

    // TODO: multiple connections from the same key?
    if (!m_connectionMapById.addElement(connection->key, connection))
    {
        delete connection;
        return -2;
    }

    stream->setReadTimeout(config.rwTimeoutInSeconds);
    stream->setWriteTimeout(config.rwTimeoutInSeconds);

    FastRPC3::SessionPTR session;

    while (ret == CONNECTION_CONTINUE)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU<uint8_t>(&readOK))
        {
        case 'A':
            // Process Answer, incoming answer for query, report to the caller...
            ret = processIncomingAnswer(connection);
            break;
        case 'Q':
            // Process Query, incoming query...
            ret = processIncomingExecutionRequest(stream, connection->key, config.keyDistFactor, &mtDone, &mtSocket, &session);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK) // <- read timeout.
                ret = CONNECTION_READ_TIMEOUT;
            else
                ret = CONNECTION_SHUTDOWN_OK;
            break;
        default:
            // Invalid protocol.
            ret = CONNECTION_INVALID_PROTOCOL;
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
    m_connectionMapById.destroyElement(connection->key);

    return ret;
}

/*
Sessions::Reason temporaryAuthentication(FastRPC3::TaskParameters *params, const std::string &userName, const std::string &domainName, const Sessions::CredentialData &authData)
{
    Sessions::Reason eReason;

    // Current Auth Domains is already checked for existence in executeRPCTask:
    auto identityManager = params->currentAuthDomains->openDomain(domainName);
    if (!identityManager)
        eReason = Sessions::REASON_INVALID_DOMAIN;
    else
    {
        Sessions::ClientDetails clientDetails;
        clientDetails.ipAddress = params->remotePeerIPAddress;
        clientDetails.tlsCommonName = params->remotePeerTLSCommonName;
        clientDetails.userAgent = "FastRPC3";
        
        eReason = identityManager->authController->authenticateCredential(params->currentMethodsHandlers->getApplicationName(),
                                     clientDetails,
                                     userName,
                                     authData.m_password,
                                     authData.m_slotId); // Authenticate in a non-persistent fashion.
        params->currentAuthDomains->releaseDomain(domainName);
    }

    return eReason;
}*/

void FastRPC3::sendRPCAnswer(FastRPC3::TaskParameters *params, const string &answer, uint8_t executionStatus)
{
    // Send a block.
    params->socketMutex->lock();
    if (params->streamBack->writeU<uint8_t>('A') && // ANSWER
        params->streamBack->writeU<uint64_t>(params->requestId) && params->streamBack->writeU<uint8_t>(executionStatus)
        && params->streamBack->writeStringEx<uint32_t>(answer.size() <= params->maxMessageSize ? answer : "", params->maxMessageSize))
    {
    }
    params->socketMutex->unlock();
}

set<string> FastRPC3::listActiveConnectionIds()
{
    return m_connectionMapById.getKeys();
}

bool FastRPC3::doesConnectionExist(const string &connectionId)
{
    return m_connectionMapById.isMember(connectionId);
}
