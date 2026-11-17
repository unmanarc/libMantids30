#pragma once

#include <Mantids30/Runtime/Logs/applog.h>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <memory>
#include <string>

#include "apiproxy.h"

namespace Mantids30::Server::WebRESTFul {

class APIProxyConfig
{
public:
    APIProxyConfig() = default;
    /**
     * @brief Creates proxy parameters from configuration without throwing parse errors.
     * @param log Optional application log for errors and configuration details.
     * @param config Proxy configuration block.
     * @param vars Variables substituted in extra header values.
     * @return Proxy parameters, or nullptr if configuration is invalid.
     */
    static std::shared_ptr<Mantids30::Server::WebRESTFul::APIProxyParameters> createAPIProxyParams(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &config,
                                                                                           const std::map<std::string, std::string> &vars = {});

private:
    /**
     * @brief Parses extra headers and substitutes configured variables.
     * @param log Application log.
     * @param headersTree Configured header values.
     * @param extraHeaders Destination header map.
     * @param vars Variables available for substitution.
     */
    static void parseExtraHeaders(Mantids30::Runtime::Logs::AppLog *log, const boost::property_tree::ptree &headersTree, std::map<std::string, std::string> &extraHeaders,
                                  const std::map<std::string, std::string> &vars);
};

} // namespace Mantids30::Server::WebRESTFul
