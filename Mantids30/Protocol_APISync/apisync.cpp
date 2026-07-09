#include "apisync.h"

#include <Mantids30/Memory/streamable_json.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_client.h>

using namespace Mantids30;
using namespace Mantids30::Program;
using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Network::Protocol;


/*
std::optional<std::string> getApplicationJWTSigningKey(Logs::AppLog *log, Mantids30::Network::Protocol::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT signing key request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT signing key request.");
        return std::nullopt;
    }

    Json::Value response = Network::Protocol::APISync::getApplicationJWTSigningKey(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.isNull() || !response.isString())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Failed to get JWT signing key from API.");
        return std::nullopt;
    }

    return response.asString();
}
*/



// ==================== MAIN FACTORY FUNCTIONS ====================

std::optional<Json::Value> Network::Protocol::APISync::getApplicationJWTConfig(Logs::AppLog *log, Mantids30::Network::Protocol::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT configuration request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT configuration request.");
        return std::nullopt;
    }

    Json::Value response = Network::Protocol::APISync::getApplicationJWTConfig(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.isNull())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Failed to get JWT configuration from API.");
        return std::nullopt;
    }

    return response;
}

std::optional<std::string> Network::Protocol::APISync::getApplicationJWTValidationKey(Logs::AppLog *log, Mantids30::Network::Protocol::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT validation key request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT validation key request.");
        return std::nullopt;
    }

    Json::Value response = Network::Protocol::APISync::getApplicationJWTValidationKey(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.isNull() || !response.isString())
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Failed to get JWT validation key from API.");
        return std::nullopt;
    }

    return response.asString();
}


Json::Value APISync::performAPISynchronizationRequest(Program::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &functionName, const Json::Value &jsonRequest, const std::string &appName,
                                               const std::string &apiKey)
{
    std::shared_ptr<Socket_Stream> connection;

    log->log0(__func__, Logs::LogLevel::INFO, "Requesting API Synchronization using '%s' for app '%s'.", functionName.c_str(), appName.c_str());

    if (proxyParameters->useTLS)
    {
        log->log0(__func__, Logs::LogLevel::DEBUG, "Using TLS connection.");

        std::shared_ptr<Socket_TLS> socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            log->log0(__func__, Logs::LogLevel::DEBUG, "Enabling certificate validation.");
            socket->setCertValidation(Socket_TLS::X509ValidationOption::VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                log->log0(__func__, Logs::LogLevel::DEBUG, "Using private CA from path: '%s'.", proxyParameters->privateCAPath.c_str());
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            log->log0(__func__, Logs::LogLevel::WARNING, "Skipping TLS certificate validation.");
            socket->setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
        }

        connection = socket;
    }
    else
    {
        log->log0(__func__, Logs::LogLevel::WARNING, "Using plain TCP connection.");
        std::shared_ptr<Socket_TCP> socket = std::make_shared<Socket_TCP>();
        connection = socket;
    }

    // Make the connection
    if (connection->connectTo(proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort))
    {
        log->log0(__func__, Logs::LogLevel::DEBUG, "Connected to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);

        HTTP::HTTPv1_Client client(connection);

        std::shared_ptr<Memory::Streams::StreamableJSON> strJSONRequest = std::make_shared<Memory::Streams::StreamableJSON>();

        *strJSONRequest->getValue() = jsonRequest;
        (*strJSONRequest->getValue())["APIKEY"] = apiKey;

        // Set POST as request method
        client.clientRequest.requestLine.setHTTPMethod("POST");
        client.clientRequest.requestLine.setRequestURI("/api/v1/" + functionName);
        client.clientRequest.getVarsBySource(HTTP::Source::GET)->addVar("APP", std::make_shared<Memory::Containers::B_Chunks>(appName));
        client.clientRequest.content.setStreamableObj(strJSONRequest);
        client.clientRequest.headers.add("Content-Type", "application/json");

        std::shared_ptr<Memory::Streams::StreamableJSON> strJSONResponse = std::make_shared<Memory::Streams::StreamableJSON>();

        client.serverResponse.setDataStreamer(strJSONResponse);

        // Make the petition...
        Mantids30::Memory::Streams::Parser::ParseResult msg;
        client.parseObject(&msg);

        if (msg == Mantids30::Memory::Streams::Parser::ParseResult::SUCCEED)
        {
            if (client.serverResponse.status.getCode() != HTTP::Status::Code::S_200_OK)
            {
                log->log0(__func__, Logs::LogLevel::ERROR, "Failed to retrieve Response. Error code: %d. = %s", static_cast<int>(client.serverResponse.status.getCode()),
                          strJSONResponse->getValue()->toStyledString().c_str());
            }
            else
            {
                log->log0(__func__, Logs::LogLevel::DEBUG, "API request to %s successful.", functionName.c_str());
                return *strJSONResponse->getValue();
            }
        }
        else
        {
            log->log0(__func__, Logs::LogLevel::ERROR, "Failed to parse API response. Error code: %d.", static_cast<int>(msg));
        }
    }
    else
    {
        log->log0(__func__, Logs::LogLevel::ERROR, "Failed to connect to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);
    }

    return {};
}

Json::Value APISync::updateAccessControlContext(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey, const Json::Value &scopes, const Json::Value &roles,
                                         const Json::Value &activities)
{
    Json::Value request;
    request["scopes"] = scopes;
    request["roles"] = roles;
    request["activities"] = activities;
    return performAPISynchronizationRequest(_log, proxyParameters, "updateAccessControlContext", request, appName, apiKey);
}

Json::Value APISync::getApplicationJWTConfig(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTConfig", Json::nullValue, appName, apiKey);
}
/*
Json::Value APISync::getApplicationJWTSigningKey(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTSigningKey", Json::nullValue, appName, apiKey);
}*/

Json::Value APISync::getApplicationJWTValidationKey(Program::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
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

    if (boost::optional<const boost::property_tree::ptree &> tlsHeaders = ptr.get_child_optional("TLS"))
    {
        checkTLSPeer = tlsHeaders->get<bool>("CheckTLSPeer", true);
        usePrivateCA = tlsHeaders->get<bool>("UsePrivateCA", false);
        privateCAPath = ptr.get<std::string>("PrivateCAPath", "");
    }
}
