#include "apiserver_clienthandler.h"

#include <cstdarg>
#include <string>

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network::Servers::Web;
using namespace std;

void APIServer_ClientHandler::log(Json::Value &jWebLog)
{
    if (logUsername.empty())
    {
        jWebLog["user"] = logUsername;
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
        config->rpcLog->logVA(logLevel, clientRequest.networkClientInfo.REMOTE_ADDR, currentSessionInfo.halfSessionId, user, domain, module, outSize, fmtLog, args);
    }

    va_end(args);
}