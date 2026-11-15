#include "fastrpc.h"
#include <Mantids30/Core/Helpers/defines.h>
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <thread>

using namespace Mantids30::Protocols::FastRPC1;
using namespace Mantids30;
using Ms = std::chrono::milliseconds;
using S = std::chrono::seconds;

void fastRPCPingerThread(FastRPC1 *obj)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "fRPC1:Pings");
#endif

    while (obj->waitPingInterval())
    {
        obj->sendPings(); // send pings to every registered client...
    }
}

Mantids30::Applications::FastRPC1Client::FastRPC1(uint32_t threadsCount, uint32_t taskQueues)
{
    m_threadPool = new Mantids30::Core::Threads::Pool::ThreadPool(threadsCount, taskQueues);
    m_threadPool->start();
    m_pinger = std::thread(fastRPCPingerThread, this);
}

FastRPC1::~FastRPC1()
{
    // Set pings to cease (the current one will continue)
    m_finished = true;

    // Notify that pings should stop now... This can take a while (while pings are cycled)...
    {
        std::unique_lock<std::mutex> lk(m_pingMutex);
        m_pingCond.notify_all();
    }

    // Wait until the loop ends...
    m_pinger.join();

    delete m_threadPool;
}

void Mantids30::Applications::FastRPC1Client::stop()
{
    m_threadPool->stop();
}

void Mantids30::Applications::FastRPC1Client::setQueuePushTimeoutInMS(const uint32_t &value)
{
    this->m_queuePushTimeoutInMS = value;
}

bool Mantids30::Applications::FastRPC1Client::addMethod(const std::string &methodName, const Mantids30::Applications::FastRPC1Client::Method &method)
{
    std::unique_lock<std::shared_mutex> lock(m_methodsMutex);
    if (m_methods.find(methodName) == m_methods.end())
    {
        // Put the method.
        m_methods[methodName] = method;
        return true;
    }
    return false;
}

void Mantids30::Applications::FastRPC1Client::sendPings()
{
    // This will create some traffic:
    std::set<std::string> keys = m_connectionsByKeyId.getKeys();
    for (const std::string &i : keys)
    {
        // Avoid to ping more hosts during program finalization...
        if (m_finished)
        {
            return;
        }
        // Run unexistant remote function
        runRemoteRPCMethod(i, "_pingNotFound_", {}, nullptr, false);
    }
}

void Mantids30::Applications::FastRPC1Client::setPingInterval(uint32_t _intvl)
{
    m_pingIntvl = _intvl;
}

uint32_t Mantids30::Applications::FastRPC1Client::getPingInterval() const
{
    return m_pingIntvl;
}

bool Mantids30::Applications::FastRPC1Client::waitPingInterval()
{
    std::unique_lock<std::mutex> lk(m_pingMutex);
    return m_pingCond.wait_for(lk, S(m_pingIntvl)) == std::cv_status::timeout;
}

boost::json::value Mantids30::Applications::FastRPC1Client::runLocalRPCMethod(std::string_view methodName, const std::string &connectionKey, const std::string &data, const std::shared_ptr<void> &context, const boost::json::value &payload, bool *found)
{
    boost::json::value r;
    std::shared_lock<std::shared_mutex> lock(m_methodsMutex);
    const std::string mName{methodName};
    if (m_methods.find(mName) != m_methods.end())
    {
        r = m_methods[mName].method(m_overwriteContext ? m_overwriteContext : m_methods[mName].context, connectionKey, payload, context, data);
        SET_OK_BOOLEAN(found, true);
    }
    else
    {
        SET_OK_BOOLEAN(found, false);
    }
    return r;
}

void Mantids30::Applications::FastRPC1Client::eventUnexpectedAnswerReceived(Mantids30::Applications::FastRPC1Client::Connection *, const std::string &) {}

int Mantids30::Applications::FastRPC1Client::processAnswer(Mantids30::Applications::FastRPC1Client::Connection *connection)
{
    uint32_t maxAlloc = m_maxMessageSize;
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
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        if (connection->pendingRequests.find(requestId) != connection->pendingRequests.end())
        {
            connection->executionStatus[requestId] = executionStatus;

            Mantids30::DataFormats::JSON::Reader reader;
            bool parsingSuccessful = reader.parse(payloadBytes, connection->answers[requestId]);
            if (parsingSuccessful)
            {
                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->cvAnswers.notify_all();
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
            eventUnexpectedAnswerReceived(connection, payloadBytes);
        }
    }

    delete[] payloadBytes;
    return 1;
}

int Mantids30::Applications::FastRPC1Client::processQuery(const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &stream, const std::string &key, const float &priority, std::shared_mutex *mtDone, std::mutex *mtSocket,
                           const std::shared_ptr<void> &context, const std::string &data)
{
    uint32_t maxAlloc = m_maxMessageSize;
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
    std::string methodName = stream->readStringEx<uint8_t>(&ok);
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

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Mantids30::DataFormats::JSON::Reader reader;
    std::shared_ptr<Mantids30::Applications::FastRPC1Client::ThreadParameters> params = std::make_shared<Mantids30::Applications::FastRPC1Client::ThreadParameters>();
    params->requestId = requestId;
    params->methodName = methodName;
    params->done = mtDone;
    params->mtSocket = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->key = key;
    params->data = data;
    params->context = context;
    params->maxMessageSize = m_maxMessageSize;

    bool parsingSuccessful = reader.parse(payloadBytes, params->payload);
    delete[] payloadBytes;

    if (!parsingSuccessful)
    {
        // Bad Incoming JSON... Disconnect
        //delete params;
        return -3;
    }
    else
    {
        params->done->lock_shared();
        if (!m_threadPool->pushTask(executeRPCTask, params, m_queuePushTimeoutInMS, priority, key))
        {
            // Can't push the task in the queue. Null answer.
            eventFullQueueDrop(params.get());
            sendRPCAnswer(params.get(), "", static_cast<uint8_t>(TaskExecutionStatus::ERR_REMOTE_QUEUE_OVERFLOW));
            params->done->unlock_shared();
            //delete params;
        }
    }
    return 1;
}

std::shared_ptr<void> Mantids30::Applications::FastRPC1Client::getOverwriteObject() const
{
    return m_overwriteContext;
}

void Mantids30::Applications::FastRPC1Client::setOverwriteObject(const std::shared_ptr<void> &newOverwriteObject)
{
    m_overwriteContext = newOverwriteObject;
}

uint32_t Mantids30::Applications::FastRPC1Client::getRWTimeout() const
{
    return m_rwTimeout;
}

void Mantids30::Applications::FastRPC1Client::setRWTimeout(uint32_t _rwTimeout)
{
    m_rwTimeout = _rwTimeout;
}

void Mantids30::Applications::FastRPC1Client::setRemoteExecutionDisconnectedTries(const uint32_t &value)
{
    m_remoteExecutionDisconnectedTries = value;
}

void Mantids30::Applications::FastRPC1Client::setRemoteExecutionTimeoutInMS(const uint32_t &value)
{
    m_remoteExecutionTimeoutInMS = value;
}

int Mantids30::Applications::FastRPC1Client::processConnection(const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &stream, const std::string &key, const Mantids30::Applications::FastRPC1Client::CallBackOnConnected &_cb_OnConnected, const float &keyDistFactor,
                                const std::shared_ptr<void> &context, const std::string &data)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "FRPC:procCNT");
#endif

    int ret = 1;

    std::shared_mutex mtDone;
    std::mutex mtSocket;

    Mantids30::Applications::FastRPC1Client::Connection *connection = new Mantids30::Applications::FastRPC1Client::Connection;
    connection->context = context;
    connection->data = data;
    connection->mtSocket = &mtSocket;
    connection->key = key;
    connection->stream = stream;

    if (!m_connectionsByKeyId.addElement(key, connection))
    {
        delete connection;
        return -2;
    }

    // Now here is connected....
    if (_cb_OnConnected.fastRPCCB_OnConnected != nullptr)
    {
        std::thread i = std::thread(_cb_OnConnected.fastRPCCB_OnConnected, key, _cb_OnConnected.context);
        i.detach();
    }

    stream->setReadTimeout(m_rwTimeout);
    stream->setWriteTimeout(m_rwTimeout);

    while (ret > 0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU<uint8_t>(&readOK))
        {
        case 'A':
            // Process Answer
            ret = processAnswer(connection);
            break;
        case 'Q':
            // Process Query
            ret = processQuery(stream, key, keyDistFactor, &mtDone, &mtSocket, context, data);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK)
            { // <- read timeout.
                ret = -101;
            }
            else
            {
                ret = 0;
            }
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
    connection->cvAnswers.notify_all();

    m_connectionsByKeyId.destroyElement(key);

    return ret;
}

void Mantids30::Applications::FastRPC1Client::executeRPCTask(const std::shared_ptr<void> &taskData)
{
    Mantids30::Applications::FastRPC1Client::ThreadParameters *params = static_cast<Mantids30::Applications::FastRPC1Client::ThreadParameters *>(taskData.get());

    bool found = false;
    boost::json::value r = (static_cast<FastRPC1 *>(params->caller))->runLocalRPCMethod(params->methodName, params->key, params->data, params->context, params->payload, &found);
    std::string output = boost::json::serialize(r);
    sendRPCAnswer(params, output, found ? static_cast<uint8_t>(TaskExecutionStatus::SUCCESS) : static_cast<uint8_t>(TaskExecutionStatus::ERR_METHOD_NOT_FOUND));
    params->done->unlock_shared();
}

void Mantids30::Applications::FastRPC1Client::sendRPCAnswer(Mantids30::Applications::FastRPC1Client::ThreadParameters *params, const std::string &answer, uint8_t executionStatus)
{
    // Send a block.
    params->mtSocket->lock();
    if (params->streamBack->writeU<uint8_t>('A') && // ANSWER
        params->streamBack->writeU<uint64_t>(params->requestId) && params->streamBack->writeU<uint8_t>(executionStatus)
        && params->streamBack->writeStringEx<uint32_t>(answer.size() <= params->maxMessageSize ? answer : "", params->maxMessageSize))
    {
    }
    params->mtSocket->unlock();
}

void Mantids30::Applications::FastRPC1Client::setMaxMessageSize(const uint32_t &value)
{
    m_maxMessageSize = value;
}

boost::json::value Mantids30::Applications::FastRPC1Client::runRemoteRPCMethod(const std::string &connectionKey, std::string_view methodName, const boost::json::value &payload, boost::json::value *error, bool retryIfDisconnected)
{
    boost::json::value r;
    const std::string mMethodName{methodName};

    std::string output = boost::json::serialize(payload);

    if (output.size() > m_maxMessageSize)
    {
        if (error)
        {
            Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
            Mantids30::DataFormats::JSON::setMember(*error, "errorId", 1);
            Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Payload exceed the Maximum Message Size.");
        }
        return r;
    }

    Mantids30::Applications::FastRPC1Client::Connection *connection;

    uint32_t _tries = 0;
    while ((connection = static_cast<Mantids30::Applications::FastRPC1Client::Connection *>(m_connectionsByKeyId.openElement(connectionKey))) == nullptr)
    {
        _tries++;
        if (_tries >= m_remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            eventRemotePeerDisconnected(connectionKey, mMethodName, payload);
            if (error)
            {
                Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                Mantids30::DataFormats::JSON::setMember(*error, "errorId", 2);
                Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Abort after remote peer not found/connected.");
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

    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        // Create authorization to be inserted:
        connection->pendingRequests.insert(requestId);
    }

    connection->mtSocket->lock();
    if (connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
        connection->stream->writeU<uint64_t>(requestId) && connection->stream->writeStringEx<uint8_t>(mMethodName) && connection->stream->writeStringEx<uint32_t>(output, m_maxMessageSize))
    {
    }
    connection->mtSocket->unlock();

    // Time to wait for answers...
    for (;;)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);

        // Process multiple signals until our answer comes...

        if (connection->cvAnswers.wait_for(lk, Ms(m_remoteExecutionTimeoutInMS)) == std::cv_status::timeout)
        {
            // break by timeout. (no answer)
            eventRemoteExecutionTimedOut(connectionKey, mMethodName, payload);
            if (error)
            {
                Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                Mantids30::DataFormats::JSON::setMember(*error, "errorId", 3);
                Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Remote Execution Timed Out: No Answer Received.");
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
                    Mantids30::DataFormats::JSON::setMember(*error, "succeed", true);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorId", 0);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Execution OK.");
                    break;
                case 3:
                    Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorId", 4);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Remote Execution Failed: Full Queue.");
                    break;
                case 4:
                    Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorId", 5);
                    Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Remote Execution Failed: Method Not Found.");
                    break;
                default:
                    Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                }
            }
            break;
        }

        if (lk.owns_lock() && connection->terminated)
        {
            if (error)
            {
                Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
                Mantids30::DataFormats::JSON::setMember(*error, "errorId", 6);
                Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Connection is terminated: No Answer Received.");
            }
            break;
        }
    }

    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    m_connectionsByKeyId.releaseElement(connectionKey);

    if (error)
    {
        if (!Mantids30::DataFormats::JSON::HAS_MEMBER(*error, "succeed"))
        {
            Mantids30::DataFormats::JSON::setMember(*error, "succeed", false);
            Mantids30::DataFormats::JSON::setMember(*error, "errorId", 99);
            Mantids30::DataFormats::JSON::setMember(*error, "errorMessage", "Unknown Error.");
        }
    }

    return r;
}

bool Mantids30::Applications::FastRPC1Client::runRemoteClose(const std::string &connectionKey)
{
    bool r = false;

    Mantids30::Applications::FastRPC1Client::Connection *connection;
    if ((connection = static_cast<Mantids30::Applications::FastRPC1Client::Connection *>(m_connectionsByKeyId.openElement(connectionKey))) != nullptr)
    {
        connection->mtSocket->lock();
        if (connection->stream->writeU<uint8_t>(0))
        {
        }
        connection->mtSocket->unlock();

        m_connectionsByKeyId.releaseElement(connectionKey);
    }
    else
    {
        eventRemotePeerDisconnected(connectionKey, "", {});
    }
    return r;
}

std::set<std::string> Mantids30::Applications::FastRPC1Client::getConnectionKeys()
{
    return m_connectionsByKeyId.getKeys();
}

bool Mantids30::Applications::FastRPC1Client::checkConnectionKey(std::string_view connectionKey)
{
    return m_connectionsByKeyId.isMember(std::string(connectionKey));
}

void Mantids30::Applications::FastRPC1Client::eventFullQueueDrop(Mantids30::Applications::FastRPC1Client::ThreadParameters *) {}

void Mantids30::Applications::FastRPC1Client::eventRemotePeerDisconnected(const std::string &, const std::string &, const boost::json::value &) {}

void Mantids30::Applications::FastRPC1Client::eventRemoteExecutionTimedOut(const std::string &, const std::string &, const boost::json::value &) {}
