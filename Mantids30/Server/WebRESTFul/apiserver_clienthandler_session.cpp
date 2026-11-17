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

boost::json::value APIServer_ClientHandler::buildDynamicRequestContext()
{
    boost::json::object session;
    const bool active = isSessionActive();
    session["isActive"] = active;
    session["isAdmin"] = active && isAdmin();
    session["roles"] = Mantids30::DataFormat::JSON::fromSet(active ? getSessionRoles() : std::set<std::string>{});
    session["scopes"] = Mantids30::DataFormat::JSON::fromSet(active ? getSessionScopes() : std::set<std::string>{});
    if (active && currentSessionInfo.authSession)
    {
        session["user"] = currentSessionInfo.authSession->getUser();
        session["domain"] = currentSessionInfo.authSession->getDomain();
        session["isImpersonation"] = currentSessionInfo.isImpersonation;
        session["impersonator"] = currentSessionInfo.authSession->getImpersonator();
        session["claims"] = currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllClaimsAsJSON();
    }

    boost::json::object client;
    client["ip"] = std::string(clientRequest.networkClientInfo.REMOTE_ADDR);
    client["tlsCN"] = clientRequest.networkClientInfo.tlsCommonName;
    client["isSecure"] = clientRequest.networkClientInfo.isSecure;
    client["userAgent"] = clientRequest.userAgent;

    boost::json::object request;
    request["method"] = clientRequest.requestLine.getHTTPMethod();
    request["uri"] = clientRequest.getURI();
    request["query"] = clientRequest.requestLine.getRequestGETVarsRawString();
    request["get"] = clientRequest.getVarsBySource(Mantids30::Protocol::HTTP::Source::GET)->toJSON();
    request["post"] = clientRequest.getVarsBySource(Mantids30::Protocol::HTTP::Source::POST)->toJSON();
    request["json"] = nullptr;
    if (clientRequest.content.getContainerType() == Mantids30::Protocol::HTTP::Content::ContainerType::JSON)
    {
        if (auto json = clientRequest.content.getJSONVars())
        {
            if (const auto *value = json->processValue())
            {
                request["json"] = *value;
            }
        }
    }
    request["headers"] = clientRequest.headers.toJSON();
    request["origin"] = clientRequest.getOrigin();
    request["referer"] = clientRequest.getReferer();

    return boost::json::object{{"session", std::move(session)}, {"client", std::move(client)}, {"request", std::move(request)}};
}
