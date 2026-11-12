#include "fastrpc3.h"
#include <Mantids30/API_EndpointsAndSessions/api_endpoints.h>
#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/DataFormat_JSON/json.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <shared_mutex>
#include <mutex>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>

using namespace Mantids30;
using namespace Network::Sockets;
using namespace Network::Protocol::FastRPC;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

boost::json::value FastRPC3::RemoteMethods::loginViaJWTToken(const std::string &jwtToken, boost::json::value *error)
{
    boost::json::value jAuthData = boost::json::object{};
    jAuthData.as_object()["jwtToken"] = jwtToken;
    return executeTask("SESSION.LOGIN", jAuthData, error, true, true);
}

boost::json::value FastRPC3::RemoteMethods::executeTask(std::string_view methodName, const boost::json::value &payload, boost::json::value *error, bool retryIfDisconnected, bool passSessionCommands, const string &extraJWTTokenAuth)
{
    boost::json::value r;
    const std::string mMethodName{methodName};

    if (!passSessionCommands && boost::starts_with(methodName, "SESSION."))
    {
        return r;
    }

    string output = boost::json::serialize(payload);

    if (output.size() > parent->config.maxMessageSize)
    {
        if (error)
        {
            Helpers::JSON::setMember(*error, "succeed", false);
            Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_PAYLOAD_TOO_LARGE));
            Helpers::JSON::setMember(*error, "errorMessage", "Payload exceed the Maximum Message Size.");
        }
        return r;
    }

    FastRPC3::Connection *connection;

    uint32_t _tries = 0;
    while ((connection = static_cast<FastRPC3::Connection *>(parent->m_connectionMapById.openElement(connectionId))) == nullptr)
    {
        _tries++;
        if (_tries >= parent->config.remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            CALLBACK(parent->rpcCallbacks.onOutgoingTaskFailureDisconnectedPeer)(connectionId, mMethodName, payload);
            if (error)
            {
                Helpers::JSON::setMember(*error, "succeed", false);
                Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_PEER_NOT_FOUND));
                Helpers::JSON::setMember(*error, "errorMessage", "Abort after remote peer not found/connected.");
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
        unique_lock<mutex> lk(connection->answersMutex);
        // Create authorization to be inserted:
        connection->pendingRequests.insert(requestId);
    }

    uint8_t flags = static_cast<uint8_t>(ExecutionFlag::NORMAL);
    if (!extraJWTTokenAuth.empty())
    {
        flags |= static_cast<uint8_t>(ExecutionFlag::EXTRAAUTH);
    }

    connection->socketMutex->lock();

    bool dataTransmitOK = true;

    if (connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
        connection->stream->writeU<uint64_t>(requestId) && connection->stream->writeU<uint8_t>(flags) && connection->stream->writeStringEx<uint8_t>(mMethodName)
        && connection->stream->writeStringEx<uint32_t>(output, parent->config.maxMessageSize))
    {
    }
    else
    {
        dataTransmitOK = false;
    }

    if ((flags & static_cast<uint8_t>(ExecutionFlag::EXTRAAUTH)) != 0)
    {
        if (dataTransmitOK && connection->stream->writeStringEx<uint32_t>(extraJWTTokenAuth, extraJWTTokenAuth.size()))
        {
        }
        else
        {
            dataTransmitOK = false;
        }
    }

    if (!dataTransmitOK)
    {
        if (error)
        {
            Helpers::JSON::setMember(*error, "succeed", false);
            Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_DATA_TRANSMISSION_FAILURE));
            Helpers::JSON::setMember(*error, "errorMessage", "Connection Failed.");
        }
        return r;
    }

    connection->socketMutex->unlock();

    // Time to wait for answers...
    for (;;)
    {
        unique_lock<mutex> lk(connection->answersMutex);

        // Process multiple signals until our answer comes...

        if (connection->answersCondition.wait_for(lk, Ms(parent->config.remoteExecutionTimeoutInMS)) == cv_status::timeout)
        {
            // break by timeout. (no answer)
            CALLBACK(parent->rpcCallbacks.onOutgoingTaskFailureTimeout)(connectionId, mMethodName, payload);

            if (error)
            {
                Helpers::JSON::setMember(*error, "succeed", false);
                Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_TIMEOUT));
                Helpers::JSON::setMember(*error, "errorMessage", "Remote Execution Timed Out: No Answer Received.");
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
                switch (static_cast<TaskExecutionStatus>(executionStatus))
                {
                case TaskExecutionStatus::SUCCESS:
                    Helpers::JSON::setMember(*error, "succeed", true);
                    Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::SUCCESS));
                    Helpers::JSON::setMember(*error, "errorMessage", "Execution OK.");
                    break;
                case TaskExecutionStatus::ERR_REMOTE_QUEUE_OVERFLOW:
                    Helpers::JSON::setMember(*error, "succeed", false);
                    Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_REMOTE_QUEUE_OVERFLOW));
                    Helpers::JSON::setMember(*error, "errorMessage", "Remote Execution Failed: Full Queue.");
                    break;
                case TaskExecutionStatus::ERR_METHOD_NOT_FOUND:
                    Helpers::JSON::setMember(*error, "succeed", false);
                    Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_METHOD_NOT_FOUND));
                    Helpers::JSON::setMember(*error, "errorMessage", "Remote Execution Failed: Method Not Found.");
                    break;
                default:
                    Helpers::JSON::setMember(*error, "succeed", false);
                }
            }
            break;
        }

        if (lk.owns_lock() && connection->terminated)
        {
            if (error)
            {
                Helpers::JSON::setMember(*error, "succeed", false);
                Helpers::JSON::setMember(*error, "errorId", static_cast<uint8_t>(TaskExecutionError::ERR_CONNECTION_LOST));
                Helpers::JSON::setMember(*error, "errorMessage", "Connection is terminated: No Answer Received.");
            }
            break;
        }
    }

    {
        unique_lock<mutex> lk(connection->answersMutex);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    parent->m_connectionMapById.releaseElement(connectionId);

    if (error)
    {
        if (!Helpers::JSON::HAS_MEMBER(*error, "succeed"))
        {
            Helpers::JSON::setMember(*error, "succeed", false);
            Helpers::JSON::setMember(*error, "errorId", 99);
            Helpers::JSON::setMember(*error, "errorMessage", "Unknown Error.");
        }
    }

    return r;
}

bool FastRPC3::RemoteMethods::logout(boost::json::value *error)
{
    boost::json::value x = parent->remote(connectionId).executeTask("SESSION.LOGOUT", {}, error, true, true);
    return Helpers::JSON::ASBOOL_D(x, false);
}

boost::json::value FastRPC3::RemoteMethods::getSSOData(boost::json::value *error)
{
    return parent->remote(connectionId).executeTask("SESSION.GETSSODATA", {}, error, true, true);
}

bool FastRPC3::RemoteMethods::close()
{
    bool r = false;

    FastRPC3::Connection *connection;
    if ((connection = static_cast<FastRPC3::Connection *>(parent->m_connectionMapById.openElement(connectionId))) != nullptr)
    {
        connection->socketMutex->lock();
        if (connection->stream->writeU<uint8_t>(0))
        {
        }
        connection->socketMutex->unlock();

        parent->m_connectionMapById.releaseElement(connectionId);
    }
    else
    {
        CALLBACK(parent->rpcCallbacks.onOutgoingTaskFailureDisconnectedPeer)(connectionId, "CLOSE", {});
    }
    return r;
}
