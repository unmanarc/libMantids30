#include <Mantids30/DataFormats/JSON/json.h>
#include  "apiserver_clienthandler.h"


using namespace Mantids30::Servers::RESTfulWebServer;
using namespace std;

void APIServer_ClientHandler::fillSessionInfo(boost::json::value &jVars)
{
    if (currentSessionInfo.authSession)
    {
        Mantids30::DataFormats::JSON::setMember(jVars, "isImpersonation", currentSessionInfo.isImpersonation);
        Mantids30::DataFormats::JSON::setMember(jVars, "impersonator", currentSessionInfo.authSession->getImpersonator());
        Mantids30::DataFormats::JSON::setMember(jVars, "halfSessionID", currentSessionInfo.halfSessionId);
        Mantids30::DataFormats::JSON::setMember(jVars, "user", currentSessionInfo.authSession->getUser());
        Mantids30::DataFormats::JSON::setMember(jVars, "domain", currentSessionInfo.authSession->getDomain());
        Mantids30::DataFormats::JSON::setMember(jVars, "loggedIn", true);
    }
    else
    {
        Mantids30::DataFormats::JSON::setMember(jVars, "loggedIn", false);
    }

    Mantids30::DataFormats::JSON::setMember(jVars, "userTLSCommonName", clientRequest.networkClientInfo.tlsCommonName);
    Mantids30::DataFormats::JSON::setMember(jVars, "userIP", clientRequest.networkClientInfo.REMOTE_ADDR);
    Mantids30::DataFormats::JSON::setMember(jVars, "userAgent", clientRequest.userAgent);
}