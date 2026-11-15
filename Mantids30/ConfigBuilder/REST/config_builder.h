#pragma once

#include <Mantids30/Servers/RESTfulWebServer/engine.h>
#include <Mantids30/Runtime/Logs/applog.h>
#include <Mantids30/Runtime/Logs/rpclog.h>
#include <boost/property_tree/ptree.hpp>

namespace Mantids30::ConfigBuilder::REST {

// Define options as bit flags
constexpr uint64_t REST_ENGINE_NOCONFIG_JWT = 1ULL << 0;      // Bit 0: Do not configure JWT
constexpr uint64_t REST_ENGINE_NO_SSL = 1ULL << 1;            // Bit 1: Do not configure SSL/TLS
constexpr uint64_t REST_ENGINE_DISABLE_RESOURCES = 1ULL << 2; // Bit 2: Disable Resources (don't look for resource path)
constexpr uint64_t REST_ENGINE_MANDATORY_SSL = 1ULL << 3;     // Bit 3: The SSL is mandatory...

class Builder
{
public:
    Builder() = default;
    static Mantids30::Servers::RESTfulWebServer::Engine *createRESTfulEngine(const boost::property_tree::ptree &ptr, const std::shared_ptr<Mantids30::Runtime::Logs::AppLog> &log,
                                                                  const std::shared_ptr<Mantids30::Runtime::Logs::RPCLog> &rpcLog, const std::string &serviceName,
                                                                  const std::string &defaultResourcePath, uint64_t options = 0, const std::map<std::string, std::string> &vars = {});

private:
    static bool handleProtocolInitializationFailure(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock);
    static bool handleClientAcceptTimeoutOccurred(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock);
    static bool handleClientConnectionLimitPerIPReached(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock);
};

} // namespace Mantids30::ConfigBuilder::REST