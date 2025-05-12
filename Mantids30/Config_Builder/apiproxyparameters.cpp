#include "apiproxyparameters.h"
#include <boost/property_tree/ptree.hpp>
#include <string>


using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30::Program;
using namespace Mantids30::Program::Config;
/*
Config example:


; ApiProxy Configuration
UseTLS true   ; Use TLS
CheckTLSPeer true   ; Check TLS peer
UsePrivateCA false   ; Do not use private CA
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
    Mantids30::Program::Logs::AppLog *log, const boost::property_tree::ptree & config)
{
    auto params = std::make_shared<ApiProxyParameters>();

    try
    {/*
        if (!config)
        {
            log->log0(__func__, Logs::LEVEL_ERR, "Property tree pointer is null.");
            return nullptr;
        }*/

        params->useTLS =config.get<bool>("UseTLS", true);
        params->checkTLSPeer =config.get<bool>("CheckTLSPeer", false);
        params->usePrivateCA =config.get<bool>("UsePrivateCA", false);
        params->remoteHost =config.get<std::string>("RemoteHost", "localhost");
        params->remotePort = static_cast<uint16_t>(config.get<int>("RemotePort", 8443));
        params->privateCAPath =config.get<std::string>("PrivateCAPath", "");

        // Parse extra headers
        if (config.find("ExtraHeaders") !=config.not_found())
            parseExtraHeaders(config.get_child("ExtraHeaders"), params->extraHeaders);
    }
    catch (const boost::property_tree::ptree_error &e)
    {
        log->log0(__func__, Logs::LEVEL_ERR, "Error parsing ApiProxyParameters: %s",  e.what());
        return nullptr;
    }
    catch (const std::exception &e)
    {
        log->log0(__func__, Logs::LEVEL_ERR, "General error parsing ApiProxyParameters: %s", e.what());
        return nullptr;
    }

    return params;
}

void ApiProxyConfig::parseExtraHeaders(
    const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders)
{
    for (const auto &pair : headersTree)
    {
        extraHeaders[pair.first] = pair.second.get_value<std::string>();
    }
}
