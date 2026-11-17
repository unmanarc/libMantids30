#include "apiproxy_config.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <type_traits>

using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30;

/*
Config example:


; APIProxy Configuration
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

TransformCookiePath false   ; When true, prepends the proxy internalPath to cookie Path attributes in responses

PrivateCAPath "/path/to/ca.pem"   ; Path to private CA (if UsePrivateCA were true)
*/
std::shared_ptr<APIProxyParameters> APIProxyConfig::createAPIProxyParams(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &config,
                                                                         const std::map<std::string, std::string> &vars)
{
    std::shared_ptr<APIProxyParameters> params = std::make_shared<APIProxyParameters>();

    // Typed reads distinguish missing options from invalid configured values.
    const auto readOption = [log](const boost::property_tree::ptree &tree, const std::string &name, auto &destination)
    {
        const auto node = tree.get_child_optional(name);
        if (!node)
        {
            return true;
        }
        using Value = std::decay_t<decltype(destination)>;
        const auto value = node->get_value_optional<Value>();
        if (!value)
        {
            if (log)
            {
                log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Invalid proxy configuration value for '%s'", name.c_str());
            }
            return false;
        }
        destination = *value;
        return true;
    };

    if (const auto expression = config.get_child_optional("MatchExpression"))
    {
        params->matchExpression = std::make_shared<RequestMatchExpression>(expression->data());
        if (!params->matchExpression->isCompiled())
        {
            if (log)
            {
                log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::ERROR, "Invalid proxy MatchExpression: %s",
                          params->matchExpression->getLastCompilerError().c_str());
            }
            return nullptr;
        }
    }
    if (!readOption(config, "UseTLS", params->useTLS)
        || !readOption(config, "RemoteHost", params->remoteHost)
        || !readOption(config, "RemotePort", params->remotePort)
        || !readOption(config, "TransformCookiePath", params->transformCookiePath))
    {
        return nullptr;
    }

    if (const auto tls = config.get_child_optional("TLS"))
    {
        params->checkTLSPeer = true;
        if (!readOption(*tls, "CheckTLSPeer", params->checkTLSPeer)
            || !readOption(*tls, "UsePrivateCA", params->usePrivateCA)
            || !readOption(*tls, "PrivateCAPath", params->privateCAPath))
        {
            return nullptr;
        }
    }
    if (log)
    {
        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG,
                  "Parsed Proxy configuration: UseTLS=%s, CheckTLSPeer=%s, "
                  "UsePrivateCA=%s, RemoteHost=%s, RemotePort=%u, PrivateCAPath=%s",
                  params->useTLS ? "true" : "false", params->checkTLSPeer ? "true" : "false", params->usePrivateCA ? "true" : "false", params->remoteHost.c_str(),
                  static_cast<unsigned int>(params->remotePort), params->privateCAPath.c_str());
    }

    if (const auto headers = config.get_child_optional("ExtraHeaders"))
    {
        parseExtraHeaders(log, *headers, params->extraHeaders, vars);
    }
    return params;
}

void APIProxyConfig::parseExtraHeaders(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders,
                                       const std::map<std::string, std::string> &vars)
{
    //   log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Starting to parse extra headers.");
    for (const auto &pair : headersTree)
    {
        std::string value = pair.second.data();

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
        //log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Parsed header: %s=%s", pair.first.c_str(), extraHeaders[pair.first].c_str());
        // Don't expose:
        //        log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Parsed header: %s=%s", pair.first.c_str(), pair.second.get_value<std::string>().c_str());
    }
    //    log->log0(__func__, Mantids30::Runtime::Logs::LogLevel::DEBUG, "Finished parsing extra headers.");
}
