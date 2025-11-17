#include "api_websocket_endpoint.h"
#include "api_websocket_connection.h"

using namespace Mantids30::API::WebSocket;

Endpoint::Endpoint()
    : connectionsByIdMap(std::make_shared<Threads::Safe::Map<std::string>>())
{}

size_t Endpoint::getActiveUserConnectionsCount(const std::string &userId) const
{
    size_t count = 0;
    auto keys = connectionsByIdMap->getKeys();
    for (const auto &sessionId : keys)
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

bool Endpoint::sendJSONToConnectionID(const std::string &sessionId, const Json::Value &v) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
    if (connection)
    {
        connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
        connectionsByIdMap->releaseElement(sessionId);
        return true;
    }
    return false;
}

bool Endpoint::sendJSONToUser(const std::string &userId, const Json::Value &v) const
{
    auto keys = connectionsByIdMap->getKeys();
    for (const auto &sessionId : keys)
    {
        WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
        if (connection)
        {
            if (connection->sessionInfo->authSession->getUser() == userId)
            {
                connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
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

size_t Endpoint::sendJSONToSubscriptionTopic(const std::string &topicId, const Json::Value &v) const
{
    size_t i = 0;
    auto keys = connectionsByIdMap->getKeys();
    for (const auto &sessionId : keys)
    {
        WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
        if (connection)
        {
            {
                std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
                if (connection->subscribedTopics.find(topicId) != connection->subscribedTopics.end())
                {
                    connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
                    i++;
                }
            }
            connectionsByIdMap->releaseElement(sessionId);
        }
    }
    return i;
}

bool Endpoint::joinTopicSubscription(const std::string &sessionId, const std::string &topicId) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
    if (connection)
    {
        bool r = true;
        {
            std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
            if (connection->subscribedTopics.size() < (*config)->maxSubscriptionTopicsPerConnection)
            {
                connection->subscribedTopics.insert(topicId);
            }
            else
            {
                r = false;
            }
        }
        connectionsByIdMap->releaseElement(sessionId);
        return r;
    }
    return false;
}

bool Endpoint::leaveTopicSubscription(const std::string &sessionId, const std::string &topicId) const
{
    WebSocketConnection *connection = static_cast<WebSocketConnection *>(connectionsByIdMap->openElement(sessionId));
    if (connection)
    {
        {
            std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
            connection->subscribedTopics.erase(topicId);
        }
        connectionsByIdMap->releaseElement(sessionId);
        return true;
    }
    return false;
}
