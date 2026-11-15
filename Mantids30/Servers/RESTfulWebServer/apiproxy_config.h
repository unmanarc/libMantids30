#pragma once

#include <Mantids30/Runtime/Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <memory>
#include <string>

#include "apiproxy.h"

namespace Mantids30::Servers::RESTfulWebServer {

class APIProxyConfig
{
public:
    APIProxyConfig() = default;
    static std::shared_ptr<Mantids30::Servers::RESTfulWebServer::APIProxyParameters> createAPIProxyParams(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &config,
                                                                                           const std::map<std::string, std::string> &vars = {});

private:
    static void parseExtraHeaders(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders,
                                  const std::map<std::string, std::string> &vars);
};

} // namespace Mantids30::Servers::RESTfulWebServer
