#include <Mantids30/Helpers/json.h>
#include  "apiserver_clienthandler.h"


using namespace Mantids30::Network::Servers::Web;
using namespace std;

void APIServer_ClientHandler::fillSessionInfo(boost::json::value &jVars)
{
    if (currentSessionInfo.authSession)
    {
        Mantids30::Helpers::JSON::setMember(jVars, "isImpersonation", currentSessionInfo.isImpersonation);
        Mantids30::Helpers::JSON::setMember(jVars, "impersonator", currentSessionInfo.authSession->getImpersonator());
        Mantids30::Helpers::JSON::setMember(jVars, "halfSessionID", currentSessionInfo.halfSessionId);
        Mantids30::Helpers::JSON::setMember(jVars, "user", currentSessionInfo.authSession->getUser());
        Mantids30::Helpers::JSON::setMember(jVars, "domain", currentSessionInfo.authSession->getDomain());
        Mantids30::Helpers::JSON::setMember(jVars, "loggedIn", true);
    }
    else
    {
        Mantids30::Helpers::JSON::setMember(jVars, "loggedIn", false);
    }

    Mantids30::Helpers::JSON::setMember(jVars, "userTLSCommonName", clientRequest.networkClientInfo.tlsCommonName);
    Mantids30::Helpers::JSON::setMember(jVars, "userIP", clientRequest.networkClientInfo.REMOTE_ADDR);
    Mantids30::Helpers::JSON::setMember(jVars, "userAgent", clientRequest.userAgent);
}