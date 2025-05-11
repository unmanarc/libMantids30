#pragma once

#include "Mantids30/Program_Logs/applog.h"
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <memory>
#include <string>

#include <Mantids30/Server_WebCore/apiproxy.h>


namespace Mantids30 {
namespace Program {
namespace Config {

class ApiProxyConfig
{
public:
    ApiProxyConfig() = default;

    static std::shared_ptr<Network::Servers::Web::ApiProxyParameters> createApiProxyParams(Mantids30::Program::Logs::AppLog *log,boost::property_tree::ptree *ptr,
                                                                    const std::string &configClassName);
private:
    static void parseExtraHeaders(const boost::property_tree::ptree &headersTree,
                                  std::map<std::string, std::string> &extraHeaders);
};

}
} // namespace Config_Builder
} // namespace Mantids30
