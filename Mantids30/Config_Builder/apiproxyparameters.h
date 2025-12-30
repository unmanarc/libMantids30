#pragma once

#include "Mantids30/Program_Logs/applog.h"
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <memory>
#include <string>

#include <Mantids30/Server_WebCore/apiproxy.h>

namespace Mantids30::Program {
namespace Config {

class ApiProxyConfig
{
public:
    ApiProxyConfig() = default;
    static std::shared_ptr<Network::Servers::Web::ApiProxyParameters> createApiProxyParams(Mantids30::Program::Logs::AppLog *log, const boost::property_tree::ptree &config,
                                                                                           const std::map<std::string, std::string> &vars = {});

private:
    static void parseExtraHeaders(Mantids30::Program::Logs::AppLog *log, const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders,
                                  const std::map<std::string, std::string> &vars);
};

} // namespace Config
} // namespace Mantids30::Program
