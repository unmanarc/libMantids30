#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>

#include "apiclienthandler.h"

namespace Mantids30 {
namespace Network {
namespace Servers {
namespace Web {

class HTMLIEngine
{
public:
    static Protocols::HTTP::Status::Codes processResourceFile(APIClientHandler *clientHandler, const std::string &sRealFullPath);

private:
    static json procJAPI_Exec(const std::string &sRealFullPath, APIClientHandler *clientHandler, const std::string &functionName, const std::string &functionInput);

    static void procResource_HTMLIEngineInclude(const std::string &sRealFullPath, std::string &fileContent, APIClientHandler *clientHandler);
    static void procResource_JProcessor(const std::string &sRealFullPath, std::string &input, APIClientHandler *clientHandler);

    static std::string procResource_HTMLIEngineJFUNC(const std::string &sRealFullPath, const std::string &scriptVarName, const std::string &functionDef, APIClientHandler *clientHandler);
    static std::string procResource_HTMLIEngineJGETVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler);
    static std::string procResource_HTMLIEngineJPOSTVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler);
    static std::string procResource_HTMLIEngineJSESSVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler);
    static std::string procResource_HTMLIEngineJVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIClientHandler *clientHandler);
    static std::string replaceByJVar(const json &value, const std::string &scriptVarName);
};

} // namespace Web
} // namespace Servers
} // namespace Network
} // namespace Mantids30
