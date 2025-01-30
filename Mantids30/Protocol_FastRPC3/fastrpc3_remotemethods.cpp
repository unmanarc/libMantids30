#include "fastrpc3.h"
#include <Mantids30/API_Monolith/methodshandler.h>
#include <Mantids30/Helpers/callbacks.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>

using namespace Mantids30;
using namespace Network::Sockets;
using namespace Network::Protocols::FastRPC;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

json FastRPC3::RemoteMethods::loginViaJWTToken( const std::string & jwtToken, json *error )
{
    json jAuthData;
    jAuthData["jwtToken"] = jwtToken;
    return executeTask( "SESSION.LOGIN", jAuthData, error, true, true);
}

json FastRPC3::RemoteMethods::executeTask(const string &methodName,
                                          const json &payload,
                                          json *error,
                                          bool retryIfDisconnected,
                                          bool passSessionCommands,
                                          const string &extraJWTTokenAuth)
{
    json r;

    if (!passSessionCommands && boost::starts_with(methodName, "SESSION."))
        return r;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    string output = Json::writeString(builder, payload);

    if (output.size() > parent->config.maxMessageSize)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = EXEC_ERR_PAYLOAD_TOO_LARGE;
            (*error)["errorMessage"] = "Payload exceed the Maximum Message Size.";
        }
        return r;
    }

    FastRPC3::Connection *connection;

    uint32_t _tries = 0;
    while ((connection = (FastRPC3::Connection *) parent->m_connectionMapById.openElement(connectionId)) == nullptr)
    {
        _tries++;
        if (_tries >= parent->config.remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            CALLBACK(parent->callbacks.onOutgoingTaskFailureDisconnectedPeer)(connectionId, methodName, payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = EXEC_ERR_PEER_NOT_FOUND;
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

    uint8_t flags = EXEC_FLAG_NORMAL;
    if (!extraJWTTokenAuth.empty()) flags|=EXEC_FLAG_EXTRAAUTH;

    connection->socketMutex->lock();

    bool dataTransmitOK = true;

    if (connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
        connection->stream->writeU<uint64_t>(requestId) &&
        connection->stream->writeU<uint8_t>(flags) &&
        connection->stream->writeStringEx<uint8_t>(methodName) &&
        connection->stream->writeStringEx<uint32_t>(output, parent->config.maxMessageSize))
    {
    }
    else
    {
        dataTransmitOK = false;
    }

    if (dataTransmitOK
        && connection->stream->writeStringEx<uint32_t>(extraJWTTokenAuth.c_str(), extraJWTTokenAuth.size())
        )
    {
    }
    else
    {
        dataTransmitOK = false;
    }

    if (!dataTransmitOK)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = EXEC_ERR_DATA_TRANSMISSION_FAILURE;
            (*error)["errorMessage"] = "Connection Failed.";
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
            CALLBACK(parent->callbacks.onOutgoingTaskFailureTimeout)(connectionId, methodName, payload);

            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = EXEC_ERR_TIMEOUT;
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
                    (*error)["errorId"] = EXEC_SUCCESS;
                    (*error)["errorMessage"] = "Execution OK.";
                    break;
                case 3:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = EXEC_ERR_REMOTE_QUEUE_OVERFLOW;
                    (*error)["errorMessage"] = "Remote Execution Failed: Full Queue.";
                    break;
                case 4:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = EXEC_ERR_METHOD_NOT_FOUND;
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
                (*error)["errorId"] = EXEC_ERR_CONNECTION_LOST;
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

    parent->m_connectionMapById.releaseElement(connectionId);

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

bool FastRPC3::RemoteMethods::logout(json *error)
{
    json x = parent->remote(connectionId).executeTask( "SESSION.LOGOUT", {}, error, true, true);
    return JSON_ASBOOL_D(x, false);
}

json FastRPC3::RemoteMethods::getSSOData(json *error)
{
    json x = parent->remote(connectionId).executeTask( "SESSION.GETSSODATA", {}, error, true, true);
    return JSON_ASBOOL_D(x, false);
}

bool FastRPC3::RemoteMethods::close()
{
    bool r = false;

    FastRPC3::Connection *connection;
    if ((connection = (FastRPC3::Connection *) parent->m_connectionMapById.openElement(connectionId)) != nullptr)
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
        CALLBACK(parent->callbacks.onOutgoingTaskFailureDisconnectedPeer)(connectionId, "CLOSE", {});
    }
    return r;
}
