#include "apisync.h"

#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_client.h>

using namespace Mantids30;
using namespace Mantids30::Program;
using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Network::Protocols;

json APISync::performAPISynchronizationRequest(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &functionName, const json &jsonRequest, const std::string &appName,
                                               const std::string &apiKey)
{
    std::shared_ptr<Socket_Stream> connection;

    log->log0(__func__, Logs::LEVEL_INFO, "Requesting API Synchronization using '%s' for app '%s'.", functionName.c_str(), appName.c_str());

    if (proxyParameters->useTLS)
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Using TLS connection.");

        auto socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "Enabling certificate validation.");
            socket->setCertValidation(Socket_TLS::CERT_X509_VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                log->log0(__func__, Logs::LEVEL_DEBUG, "Using private CA from path: '%s'.", proxyParameters->privateCAPath.c_str());
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            log->log0(__func__, Logs::LEVEL_WARN, "Skipping TLS certificate validation.");
            socket->setCertValidation(Socket_TLS::CERT_X509_NOVALIDATE);
        }

        connection = socket;
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_WARN, "Using plain TCP connection.");
        auto socket = std::make_shared<Socket_TCP>();
        connection = socket;
    }

    // Make the connection
    if (connection->connectTo(proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort))
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Connected to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);

        HTTP::HTTPv1_Client client(connection);

        auto strJSONRequest = std::make_shared<Memory::Streams::StreamableJSON>();

        *strJSONRequest->getValue() = jsonRequest;
        (*strJSONRequest->getValue())["APIKEY"] = apiKey;

        // Set POST as request method
        client.clientRequest.requestLine.setHTTPMethod("POST");
        client.clientRequest.requestLine.setRequestURI("/api/v1/" + functionName);
        client.clientRequest.getVars(HTTP::VARS_GET)->addVar("APP", std::make_shared<Memory::Containers::B_Chunks>(appName));
        client.clientRequest.content.setStreamableObj(strJSONRequest);
        client.clientRequest.headers.add("Content-Type", "application/json");

        auto strJSONResponse = std::make_shared<Memory::Streams::StreamableJSON>();

        client.serverResponse.setDataStreamer(strJSONResponse);

        // Make the petition...
        Mantids30::Memory::Streams::Parser::ErrorMSG msg;
        client.parseObject(&msg);

        if (msg == Mantids30::Memory::Streams::Parser::PARSING_SUCCEED)
        {
            if (client.serverResponse.status.getCode() != HTTP::Status::S_200_OK)
            {
                log->log0(__func__, Logs::LEVEL_ERR, "Failed to retrieve Response. Error code: %d. = %s", static_cast<int>(client.serverResponse.status.getCode()),
                          strJSONResponse->getValue()->toStyledString().c_str());
            }
            else
            {
                log->log0(__func__, Logs::LEVEL_DEBUG, "API request to %s successful.", functionName.c_str());
                return *strJSONResponse->getValue();
            }
        }
        else
        {
            log->log0(__func__, Logs::LEVEL_ERR, "Failed to parse API response. Error code: %d.", static_cast<int>(msg));
        }
    }
    else
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Failed to connect to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);
    }

    return {};
}

json APISync::updateAccessControlContext(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey, const json &scopes, const json &roles,
                                         const json &activities)
{
    json request;
    request["scopes"] = scopes;
    request["roles"] = roles;
    request["activities"] = activities;
    return performAPISynchronizationRequest(_log, proxyParameters, "updateAccessControlContext", request, appName, apiKey);
}

json APISync::getApplicationJWTConfig(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTConfig", Json::nullValue, appName, apiKey);
}
/*
json APISync::getApplicationJWTSigningKey(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTSigningKey", Json::nullValue, appName, apiKey);
}*/

json APISync::getApplicationJWTValidationKey(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTValidationKey", Json::nullValue, appName, apiKey);
}

void APISync::APISyncParameters::loadFromInfoTree(const boost::property_tree::ptree &ptr)
{
    apiSyncHost = ptr.get<std::string>("APISyncHost", "");

    if (apiSyncHost.empty())
    {
        return;
    }

    useTLS = ptr.get<bool>("UseTLS", true);
    apiSyncPort = ptr.get<uint16_t>("APISyncPort", 7081);

    if (auto tlsHeaders = ptr.get_child_optional("TLS"))
    {
        checkTLSPeer = tlsHeaders->get<bool>("CheckTLSPeer", true);
        usePrivateCA = tlsHeaders->get<bool>("UsePrivateCA", false);
        privateCAPath = ptr.get<std::string>("PrivateCAPath", "");
    }
}
