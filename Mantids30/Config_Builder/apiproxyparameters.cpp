#include "apiproxyparameters.h"
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>

using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30::Program;
using namespace Mantids30::Program::Config;
/*
Config example:


; ApiProxy Configuration
UseTLS true   ; Use TLS
TLS
{
    CheckTLSPeer true   ; Check TLS peer
    UsePrivateCA false   ; Do not use private CA
}
RemoteHost "example.com"   ; Remote host
RemotePort 443   ; Remote port

ExtraHeaders
{
   Authorization "Bearer token123"   ; Authorization token
   Custom-Header "value"   ; Custom header value
}

PrivateCAPath "/path/to/ca.pem"   ; Path to private CA (if UsePrivateCA were true)

*/
std::shared_ptr<ApiProxyParameters> ApiProxyConfig::createApiProxyParams(
    Mantids30::Program::Logs::AppLog * log, const boost::property_tree::ptree &config, const std::map<std::string, std::string> &vars)
{
    std::shared_ptr<ApiProxyParameters> params = std::make_shared<ApiProxyParameters>();

    try
    {
        log->log0(__func__, Logs::LEVEL_DEBUG, "Creating ApiProxyParameters.");
        params->useTLS = config.get<bool>("UseTLS", true);

        if (auto tlsHeaders = config.get_child_optional("TLS"))
        {
            params->checkTLSPeer = tlsHeaders->get<bool>("CheckTLSPeer", false);
            params->usePrivateCA = tlsHeaders->get<bool>("UsePrivateCA", false);
        }

        params->remoteHost = config.get<std::string>("RemoteHost", "localhost");
        params->remotePort = static_cast<uint16_t>(config.get<int>("RemotePort", 8443));
        params->privateCAPath = config.get<std::string>("PrivateCAPath", "");

        log->log0(__func__,
                  Logs::LEVEL_DEBUG,
                  "Parsed configuration: UseTLS=%s, CheckTLSPeer=%s, " "UsePrivateCA=%s, RemoteHost=%s, RemotePort=%u, PrivateCAPath=%s",
                  params->useTLS ? "true" : "false",
                  params->checkTLSPeer ? "true" : "false",
                  params->usePrivateCA ? "true" : "false",
                  params->remoteHost.c_str(),
                  static_cast<unsigned int>(params->remotePort),
                  params->privateCAPath.c_str());

        // Parse extra headers
        if (auto extraHeaders = config.get_child_optional("ExtraHeaders"))
        {
            parseExtraHeaders(log, *extraHeaders, params->extraHeaders, vars);
        }
    }
    catch (const boost::property_tree::ptree_error &e)
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Error parsing ApiProxyParameters: %s", e.what());
        return nullptr;
    }
    catch (const std::exception &e)
    {
        log->log0(__func__, Logs::LEVEL_ERR, "General error parsing ApiProxyParameters: %s", e.what());
        return nullptr;
    }

    log->log0(__func__, Logs::LEVEL_DEBUG, "Successfully created ApiProxyParameters.");
    return params;
}

void ApiProxyConfig::parseExtraHeaders(
    Mantids30::Program::Logs::AppLog * log, const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders, const std::map<std::string, std::string> &vars)
{
    log->log0(__func__, Logs::LEVEL_DEBUG, "Starting to parse extra headers.");
    for (const auto &pair : headersTree)
    {
        std::string value = pair.second.get_value<std::string>();

        // Replace %VARIABLE% with the corresponding value from vars
        for (const auto &varPair : vars)
        {
            const std::string varPlaceholder = "%" + varPair.first + "%";
            size_t pos = 0;
            while ((pos = value.find(varPlaceholder, pos)) != std::string::npos)
            {
                value.replace(pos, varPlaceholder.length(), varPair.second);
                pos += varPair.second.length(); // Move past the replaced text
            }
        }

        extraHeaders[pair.first] = value;
        // Expose vars:
        //log->log0(__func__, Logs::LEVEL_DEBUG, "Parsed header: %s=%s", pair.first.c_str(), extraHeaders[pair.first].c_str());
        // Don't expose:
        log->log0(__func__, Logs::LEVEL_DEBUG, "Parsed header: %s=%s", pair.first.c_str(), pair.second.get_value<std::string>().c_str());
    }
    log->log0(__func__, Logs::LEVEL_DEBUG, "Finished parsing extra headers.");
}
