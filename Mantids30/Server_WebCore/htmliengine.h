#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <regex>

#include "apiserver_clienthandler.h"

namespace Mantids30::Network::Servers::Web {

class HTMLIEngine
{
public:
    static Protocols::HTTP::Status::Codes processResourceFile(APIServer_ClientHandler *clientHandler, const std::string &sRealFullPath);

private:
    static json procJAPI_Exec(const std::string &sRealFullPath, APIServer_ClientHandler *clientHandler, const std::string &functionName, const std::string &functionInput);

    static void procResource_HTMLIEngineInclude(const std::string &sRealFullPath, const std::string &contentType, std::string &fileContent, APIServer_ClientHandler *clientHandler);
    static void procResource_JProcessor(const std::string &sRealFullPath, std::string &input, APIServer_ClientHandler *clientHandler);

    static std::string procResource_HTMLIEngineJFUNC(const std::string &sRealFullPath, const std::string &scriptVarName, const std::string &functionDef, APIServer_ClientHandler *clientHandler, bool useHTMLFrame);
    static std::string procResource_HTMLIEngineJGETVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIServer_ClientHandler *clientHandler, bool useHTMLFrame);
    static std::string procResource_HTMLIEngineJPOSTVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIServer_ClientHandler *clientHandler, bool useHTMLFrame);
    static std::string procResource_HTMLIEngineJSESSVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIServer_ClientHandler *clientHandler, bool useHTMLFrame);
    static std::string procResource_HTMLIEngineJVAR(const std::string &scriptVarName, const std::string &varName, const std::string &sRealFullPath, APIServer_ClientHandler *clientHandler, bool useHTMLFrame);
    static std::string replaceByJVar(const json &value, const std::string &scriptVarName, bool useHTMLFrame);

    static void iProcResource_JProcessor(std::string& input, const std::regex &re, const std::string &sRealFullPath, APIServer_ClientHandler* clientHandler, bool useHTMLFrame);

    static void iProcResource_HTMLIEngineInclude(const std::string &sRealFullPath, std::string &fileContent, APIServer_ClientHandler *clientHandler, const boost::regex & exStaticText);

};

} // namespace Mantids30::Network::Servers::Web
