#include <Mantids30/DataFormat/JSON/json.h>
#include "apiserver_clienthandler.h"


using namespace Mantids30::Server::WebRESTFul;
using namespace std;

void APIServer_ClientHandler::fillSessionInfo(boost::json::value &jVars)
{
    if (currentSessionInfo.authSession)
    {
        Mantids30::DataFormat::JSON::setMember(jVars, "isImpersonation", currentSessionInfo.isImpersonation);
        Mantids30::DataFormat::JSON::setMember(jVars, "impersonator", currentSessionInfo.authSession->getImpersonator());
        Mantids30::DataFormat::JSON::setMember(jVars, "halfSessionID", currentSessionInfo.halfSessionId);
        Mantids30::DataFormat::JSON::setMember(jVars, "user", currentSessionInfo.authSession->getUser());
        Mantids30::DataFormat::JSON::setMember(jVars, "domain", currentSessionInfo.authSession->getDomain());
        Mantids30::DataFormat::JSON::setMember(jVars, "loggedIn", true);
    }
    else
    {
        Mantids30::DataFormat::JSON::setMember(jVars, "loggedIn", false);
    }

    Mantids30::DataFormat::JSON::setMember(jVars, "userTLSCommonName", clientRequest.networkClientInfo.tlsCommonName);
    Mantids30::DataFormat::JSON::setMember(jVars, "userIP", clientRequest.networkClientInfo.REMOTE_ADDR);
    Mantids30::DataFormat::JSON::setMember(jVars, "userAgent", clientRequest.userAgent);
}