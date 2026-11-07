#include "api_websocket_endpoint.h"
#include "api_websocket_connection.h"

using namespace Mantids30::API::WebSocket;

Endpoint::Endpoint()
    : connectionsByIdMap(std::make_shared<Threads::Safe::Map<std::string>>())
{}

size_t Endpoint::getActiveUserConnectionsCount(std::string_view userId) const
{
    size_t count = 0;
    std::set<std::string> keys = connectionsByIdMap->getKeys();
    for (const std::string &sessionId : keys)
    {
        WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
        if (connection)
        {
            if (connection->sessionInfo->authSession->getUser() == userId)
            {
                count++;
            }
            connectionsByIdMap->releaseElement(sessionId);
        }
    }
    return count;
}

bool Endpoint::sendJSONToConnectionID(std::string_view sessionId, const boost::json::value &v) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement((std::string)sessionId));
    if (connection)
    {
        connection->webSocketHTTPServer->sendWebSocketText(boost::json::serialize(v));
        connectionsByIdMap->releaseElement((std::string)sessionId);
        return true;
    }
    return false;
}

bool Endpoint::sendJSONToUser(std::string_view userId, const boost::json::value &v) const
{
    std::set<std::string> keys = connectionsByIdMap->getKeys();
    for (const std::string &sessionId : keys)
    {
        WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
        if (connection)
        {
            if (connection->sessionInfo->authSession->getUser() == userId)
            {
                connection->webSocketHTTPServer->sendWebSocketText(boost::json::serialize(v));
                connectionsByIdMap->releaseElement(sessionId);
                return true;
            }
            else
            {
                connectionsByIdMap->releaseElement(sessionId);
            }
        }
    }
    return false;
}

size_t Endpoint::sendJSONToSubscriptionTopic(std::string_view topicId, const boost::json::value &v) const
{
    size_t i = 0;
    std::set<std::string> keys = connectionsByIdMap->getKeys();
    for (const std::string &sessionId : keys)
    {
        WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
        if (connection)
        {
            {
                std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
                if (connection->subscribedTopics.find((std::string)topicId) != connection->subscribedTopics.end())
                {
                    connection->webSocketHTTPServer->sendWebSocketText(boost::json::serialize(v));
                    i++;
                }
            }
            connectionsByIdMap->releaseElement(sessionId);
        }
    }
    return i;
}

bool Endpoint::joinTopicSubscription(std::string_view sessionId, std::string_view topicId) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement((std::string)sessionId));
    if (connection)
    {
        bool r = true;
        {
            std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
            if (connection->subscribedTopics.size() < (*config)->maxSubscriptionTopicsPerConnection)
            {
                connection->subscribedTopics.insert((std::string)topicId);
            }
            else
            {
                r = false;
            }
        }
        connectionsByIdMap->releaseElement((std::string)sessionId);
        return r;
    }
    return false;
}

bool Endpoint::leaveTopicSubscription(std::string_view sessionId, std::string_view topicId) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement((std::string)sessionId));
    if (connection)
    {
        {
            std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
            connection->subscribedTopics.erase((std::string)topicId);
        }
        connectionsByIdMap->releaseElement((std::string)sessionId);
        return true;
    }
    return false;
}
