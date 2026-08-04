#include "apiserver_clienthandler.h"

#include <json/value.h>

using namespace Mantids30::Network::Servers::Web;
using namespace std;

void APIServer_ClientHandler::fillSessionInfo(Json::Value &jVars)
{
    if (currentSessionInfo.authSession)
    {
        jVars["isImpersonation"] = currentSessionInfo.isImpersonation;
        jVars["impersonator"] = currentSessionInfo.authSession->getImpersonator();
        jVars["halfSessionID"] = currentSessionInfo.halfSessionId;
        jVars["user"] = currentSessionInfo.authSession->getUser();
        jVars["domain"] = currentSessionInfo.authSession->getDomain();
        jVars["loggedIn"] = true;
    }
    else
    {
        jVars["loggedIn"] = false;
    }

    jVars["userTLSCommonName"] = clientRequest.networkClientInfo.tlsCommonName;
    jVars["userIP"] = clientRequest.networkClientInfo.REMOTE_ADDR;
    jVars["userAgent"] = clientRequest.userAgent;
}