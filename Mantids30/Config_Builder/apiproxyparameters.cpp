#include "apiproxyparameters.h"
#include <boost/property_tree/ptree.hpp>
#include <string>


using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30::Program;
using namespace Mantids30::Program::Config;
/*
Config example:

{
  "ApiProxy": {
    "useTLS": true,
    "checkTLSPeer": true,
    "usePrivateCA": false,
    "remoteHost": "example.com",
    "remotePort": 443,
    "privateCAPath": "/path/to/ca.pem",
    "extraHeaders": {
      "Authorization": "Bearer token123",
      "Custom-Header": "value"
    }
  }
}

*/

std::shared_ptr<ApiProxyParameters> ApiProxyConfig::createApiProxyParams(
    Mantids30::Program::Logs::AppLog *log, boost::property_tree::ptree *ptr, const std::string &configClassName)
{
    auto params = std::make_shared<ApiProxyParameters>();

    try
    {
        if (!ptr)
        {
            log->log0(__func__, Logs::LEVEL_ERR, "Property tree pointer is null.");
            return nullptr;
        }

        const boost::property_tree::ptree &configTree = ptr->get_child(configClassName);

        params->useTLS = configTree.get<bool>("useTLS", true);
        params->checkTLSPeer = configTree.get<bool>("checkTLSPeer", false);
        params->usePrivateCA = configTree.get<bool>("usePrivateCA", false);
        params->remoteHost = configTree.get<std::string>("remoteHost", "localhost");
        params->remotePort = static_cast<uint16_t>(configTree.get<int>("remotePort", 8443));
        params->privateCAPath = configTree.get<std::string>("privateCAPath", "");

        // Parse extra headers
        if (configTree.find("extraHeaders") != configTree.not_found())
            parseExtraHeaders(configTree.get_child("extraHeaders"), params->extraHeaders);
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
