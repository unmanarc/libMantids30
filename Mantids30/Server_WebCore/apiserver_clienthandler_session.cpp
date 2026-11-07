#include <Mantids30/DataFormat_JSON/json.h>
#include  "apiserver_clienthandler.h"


using namespace Mantids30::Network::Servers::Web;
using namespace std;

void APIServer_ClientHandler::fillSessionInfo(boost::json::value &jVars)
{
    if (currentSessionInfo.authSession)
    {
        Helpers::JSON::setMember(jVars, "isImpersonation", currentSessionInfo.isImpersonation);
        Helpers::JSON::setMember(jVars, "impersonator", currentSessionInfo.authSession->getImpersonator());
        Helpers::JSON::setMember(jVars, "halfSessionID", currentSessionInfo.halfSessionId);
        Helpers::JSON::setMember(jVars, "user", currentSessionInfo.authSession->getUser());
        Helpers::JSON::setMember(jVars, "domain", currentSessionInfo.authSession->getDomain());
        Helpers::JSON::setMember(jVars, "loggedIn", true);
    }
    else
    {
        Helpers::JSON::setMember(jVars, "loggedIn", false);
    }

    Helpers::JSON::setMember(jVars, "userTLSCommonName", clientRequest.networkClientInfo.tlsCommonName);
    Helpers::JSON::setMember(jVars, "userIP", clientRequest.networkClientInfo.REMOTE_ADDR);
    Helpers::JSON::setMember(jVars, "userAgent", clientRequest.userAgent);
}