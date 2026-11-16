#include "apisync.h"

#include <Mantids30/DataFormat/JSON/streamable_json.h>
#include <Mantids30/Network/Sockets/socket_tls.h>
#include <Mantids30/Protocol/HTTP/httpv1_base.h>
#include <Mantids30/Protocol/HTTP/httpv1_client.h>
#include <string>

using namespace Mantids30;
using namespace Mantids30::Network::Sockets;
using namespace Mantids30::Client;


/*
std::optional<std::string> getApplicationJWTSigningKey(Mantids30::Runtime::Logs::AppLog *log, Mantids30::Client::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT signing key request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT signing key request.");
        return std::nullopt;
    }

    boost::json::value response = Mantids30::Client::APISync::getApplicationJWTSigningKey(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.is_null() || !response.is_string())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to get JWT signing key from API.");
        return std::nullopt;
    }

    return response.asString();
}
*/



// ==================== MAIN FACTORY FUNCTIONS ====================

std::optional<boost::json::value> Mantids30::Client::APISync::getApplicationJWTConfig(Mantids30::Runtime::Logs::AppLog *log, Mantids30::Client::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT configuration request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT configuration request.");
        return std::nullopt;
    }

    boost::json::value response = Mantids30::Client::APISync::getApplicationJWTConfig(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.is_null())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to get JWT configuration from API.");
        return std::nullopt;
    }

    return response;
}

std::optional<std::string> Mantids30::Client::APISync::getApplicationJWTValidationKey(Mantids30::Runtime::Logs::AppLog *log, Mantids30::Client::APISync::APISyncParameters *proxyParameters, const std::map<std::string, std::string> &vars)
{
    const std::map<std::string, std::string>::const_iterator itAppName = vars.find("APP");
    if (itAppName == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APP' variable in JWT validation key request.");
        return std::nullopt;
    }

    const std::map<std::string, std::string>::const_iterator itApiKey = vars.find("APIKEY");
    if (itApiKey == vars.end())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Missing 'APIKEY' variable in JWT validation key request.");
        return std::nullopt;
    }

    boost::json::value response = Mantids30::Client::APISync::getApplicationJWTValidationKey(log, proxyParameters, itAppName->second, itApiKey->second);
    if (response.is_null() || !response.is_string())
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to get JWT validation key from API.");
        return std::nullopt;
    }

    return std::string(Mantids30::DataFormat::JSON::ASSTRING_D(response, ""));
}


boost::json::value APISync::performAPISynchronizationRequest(Mantids30::Runtime::Logs::AppLog *log, APISyncParameters *proxyParameters, const std::string &functionName, const boost::json::value &jsonRequest, const std::string &appName,
                                               const std::string &apiKey)
{
    std::shared_ptr<Socket_Stream> connection;

    log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::INFO, "Requesting API Synchronization using '%s' for app '%s'.", functionName.c_str(), appName.c_str());

    if (proxyParameters->useTLS)
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Using TLS connection.");

        std::shared_ptr<Socket_TLS> socket = std::make_shared<Socket_TLS>();

        if (proxyParameters->checkTLSPeer)
        {
            log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Enabling certificate validation.");
            socket->setCertValidation(Socket_TLS::X509ValidationOption::VALIDATE);
            socket->tlsKeys.setUseSystemCertificates(!proxyParameters->usePrivateCA);
            if (proxyParameters->usePrivateCA)
            {
                log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Using private CA from path: '%s'.", proxyParameters->privateCAPath.c_str());
                socket->tlsKeys.loadCAFromPEMFile(proxyParameters->privateCAPath);
            }
        }
        else
        {
            log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::WARNING, "Skipping TLS certificate validation.");
            socket->setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
        }

        connection = socket;
    }
    else
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::WARNING, "Using plain TCP connection.");
        std::shared_ptr<Socket_TCP> socket = std::make_shared<Socket_TCP>();
        connection = socket;
    }

    // Make the connection
    if (connection->connectTo(proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort))
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Connected to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);

        Mantids30::Protocol::HTTP::HTTPv1_Client client(connection);

        std::shared_ptr<Mantids30::DataFormat::JSON::StreamableJSON> strJSONRequest = std::make_shared<Mantids30::DataFormat::JSON::StreamableJSON>();

        *strJSONRequest->getValue() = jsonRequest;
        Mantids30::DataFormat::JSON::setMember(*strJSONRequest->getValue(), "APIKEY", apiKey);

        // Set POST as request method
        client.clientRequest.requestLine.setHTTPMethod("POST");
        client.clientRequest.requestLine.setRequestURI("/api/v1/" + functionName);
        client.clientRequest.getVarsBySource(Mantids30::Protocol::HTTP::Source::GET)->addVar("APP", std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>(appName));
        client.clientRequest.content.setStreamableObject(strJSONRequest);
        client.clientRequest.headers.add("Content-Type", "application/json");

        std::shared_ptr<Mantids30::DataFormat::JSON::StreamableJSON> strJSONResponse = std::make_shared<Mantids30::DataFormat::JSON::StreamableJSON>();

        client.serverResponse.setContentDataStreamer(strJSONResponse);

        // Make the petition...
        Mantids30::Core::Memory::Streams::Parser::ParseResult msg;
        client.parseObject(&msg);

        if (msg == Mantids30::Core::Memory::Streams::Parser::ParseResult::SUCCEED)
        {
            if (client.serverResponse.status.getCode() != Mantids30::Protocol::HTTP::Status::Code::S_200_OK)
            {
                log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to retrieve Response. Error code: %d. = %s", static_cast<int>(client.serverResponse.status.getCode()),
                          (boost::json::serialize(*strJSONResponse->getValue())).c_str());
            }
            else
            {
                log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "API request to %s successful.", functionName.c_str());
                return *strJSONResponse->getValue();
            }
        }
        else
        {
            log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to parse API response. Error code: %d.", static_cast<int>(msg));
        }
    }
    else
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Failed to connect to API server at %s:%d.", proxyParameters->apiSyncHost.c_str(), proxyParameters->apiSyncPort);
    }

    return {};
}

boost::json::value APISync::updateAccessControlContext(Mantids30::Runtime::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey, const boost::json::value &scopes, const boost::json::value &roles,
                                         const boost::json::value &activities)
{
    boost::json::value request = boost::json::object{};
    request.as_object()["scopes"] = scopes;
    request.as_object()["roles"] = roles;
    request.as_object()["activities"] = activities;
    return performAPISynchronizationRequest(_log, proxyParameters, "updateAccessControlContext", request, appName, apiKey);
}

boost::json::value APISync::getApplicationJWTConfig(Mantids30::Runtime::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTConfig", boost::json::value{}, appName, apiKey);
}
/*
boost::json::value APISync::getApplicationJWTSigningKey(Mantids30::Runtime::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTSigningKey", boost::json::value{}, appName, apiKey);
}*/

boost::json::value APISync::getApplicationJWTValidationKey(Mantids30::Runtime::Logs::AppLog *_log, APISyncParameters *proxyParameters, const std::string &appName, const std::string &apiKey)
{
    return performAPISynchronizationRequest(_log, proxyParameters, "getApplicationJWTValidationKey", boost::json::value{}, appName, apiKey);
}

void Mantids30::Client::APISync::APISyncParameters::loadFromInfoTree(const boost::property_tree::ptree &ptr)
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
