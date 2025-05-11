#pragma once

#include <Mantids30/Program_Logs/applog.h>
#include <Mantids30/Server_RESTfulWebAPI/engine.h>
#include <boost/property_tree/ptree.hpp>

namespace Mantids30 {
namespace Program {
namespace Config {

// Define options as bit flags
constexpr uint64_t REST_ENGINE_NO_JWT = 1ULL << 0;            // Bit 0: Do not configure JWT
constexpr uint64_t REST_ENGINE_NO_SSL = 1ULL << 1;            // Bit 1: Do not configure SSL/TLS
constexpr uint64_t REST_ENGINE_DISABLE_RESOURCES = 1ULL << 2; // Bit 2: Disable Resources (don't look for resource path)
constexpr uint64_t REST_ENGINE_MANDATORY_SSL = 1ULL << 3;     // Bit 3: The SSL is mandatory...

class RESTful_Engine
{
public:
    RESTful_Engine() = default;
    static Network::Servers::RESTful::Engine *createRESTfulEngine(boost::property_tree::ptree *ptr,
                                                                  Mantids30::Program::Logs::AppLog *log,
                                                                  Logs::RPCLog *rpcLog,
                                                                  const std::string & serviceName,
                                                                  const std::string & defaultResourcePath,
                                                                  uint64_t options = 0);

private:
    static bool handleProtocolInitializationFailure(void * data, std::shared_ptr<Network::Sockets::Socket_Stream> sock );
    static bool handleClientAcceptTimeoutOccurred(void *data, std::shared_ptr<Network::Sockets::Socket_Stream> sock);
    static bool handleClientConnectionLimitPerIPReached(void *data, std::shared_ptr<Network::Sockets::Socket_Stream> sock);

};

} // namespace Config
} // namespace Program
} // namespace Mantids30
