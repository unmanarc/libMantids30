#include "apiserver_clienthandler.h"

#include <boost/algorithm/string/replace.hpp>
#include <cstdarg>
#include <string>

using namespace Mantids30::Runtime::Logs;
using namespace Mantids30::Server::WebRESTFul;
using namespace std;

Mantids30::Protocol::HTTP::APIReturn APIServer_ClientHandler::handleAPIRequestFromFile(std::string sourcePath, const std::string &baseApiUrl,
                                                                                   const uint32_t &apiVersion, const std::string &httpMethodType,
                                                                                   const std::string &endpointName, const boost::json::value &postParameters)
{
    struct RestoreCallContext
    {
        std::string &current;
        std::string &previous;
        bool &embedded;
        bool previousEmbedded;
        ~RestoreCallContext()
        {
            current.swap(previous);
            embedded = previousEmbedded;
        }
    };

    m_logSourcePath.swap(sourcePath);
    const RestoreCallContext restore{m_logSourcePath, sourcePath, m_isEmbeddedAPICall, m_isEmbeddedAPICall};
    m_isEmbeddedAPICall = true;
    return handleAPIRequest(baseApiUrl, apiVersion, httpMethodType, endpointName, postParameters);
}

void APIServer_ClientHandler::log(boost::property_tree::ptree &jWebLog)
{
    if (logUsername.empty())
    {
        jWebLog.put("user", logUsername);
    }

    config->webLog->log(jWebLog);
}

void APIServer_ClientHandler::log(LogLevel logLevel, const std::string &module, const uint32_t &outSize, const char *fmtLog, ...)
{
    va_list args;
    va_start(args, fmtLog);

    std::string user, domain;

    if (currentSessionInfo.authSession)
    {
        if (!currentSessionInfo.authSession->getImpersonator().empty())
        {
            user = currentSessionInfo.authSession->getUser() + "<-" + currentSessionInfo.authSession->getImpersonator();
        }
        else
        {
            user = currentSessionInfo.authSession->getUser();
        }

        domain = currentSessionInfo.authSession->getDomain();
    }

    if (config->rpcLog)
    {
        // Escape percent signs because the source becomes part of a printf format.
        std::string format = fmtLog;
        if (!m_logSourcePath.empty())
        {
            format = "[source=" + boost::replace_all_copy(m_logSourcePath, "%", "%%") + "] " + format;
        }
        config->rpcLog->logVA(logLevel, clientRequest.networkClientInfo.REMOTE_ADDR, currentSessionInfo.halfSessionId, user, domain, module, outSize, format.c_str(), args);
    }

    va_end(args);
}
