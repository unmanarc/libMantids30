#include "clienthandler.h"

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

Network::Protocols::HTTP::Status::Codes ClientHandler::sessionStart()
{
    // Check for the authorization bearer token...
    string headerBearerToken = clientRequest.getAuthorizationBearer();

    // Take the auth token from the cookie (if exist)...
    string cookieBearerToken = clientRequest.getCookie("AccessToken");

    // In JWT apps, the thing should work as follow:
    // 1. the IAM will auth us in with a POST callback
    // 2. the post callback will set up the cookie (JS accessible) to our application
    // 3. the application should drop the cookie and install the token in the JS itself or in the browser storage
    //    and send the header with the cookie.

    // * CSRF should not work because you can't setup the header from outside. So, you don't require any other anti-CSRF
    //   measure.

    // We may still want to support token cookies because the application can have non API things that will login that way...
    // by example the app resources...

    if (!headerBearerToken.empty())
    {
        // First we check the header, if the header have a token, continue with it
        if ( this->config->dynamicTokenValidator!=nullptr )
        {
            m_JWTHeaderTokenVerified = this->config->dynamicTokenValidator(headerBearerToken,clientRequest.headers.getOptionValueStringByName("x-api-key"),&m_JWTToken);
        }
        else
        {
            m_JWTHeaderTokenVerified = verifyToken(headerBearerToken);
        }
    }
    else if (!cookieBearerToken.empty())
    {
        // if not, continue with the cookie.
        if ( this->config->dynamicTokenValidator!=nullptr )
        {
            m_JWTCookieTokenVerified = this->config->dynamicTokenValidator(cookieBearerToken,clientRequest.headers.getOptionValueStringByName("x-api-key"),&m_JWTToken);
        }
        else
        {
            m_JWTCookieTokenVerified = verifyToken(cookieBearerToken);
        }
    }

    if ( isSessionActive() )
    {
        // If verification is successful and the token is valid, populate user data.
        // Extract and store user-related information from the token.
        // Create a temporary session for the REST request...
        m_currentSessionInfo.authSession = make_shared<Mantids30::Sessions::Session>(m_JWTToken);
        m_currentSessionInfo.halfSessionId = m_JWTToken.getJwtId();
        m_currentSessionInfo.sessionId = m_JWTToken.getJwtId();
        m_currentSessionInfo.isImpersonation = !(m_currentSessionInfo.authSession->getImpersonator().empty());
    }

    return Protocols::HTTP::Status::S_200_OK;
}

void ClientHandler::sessionCleanup()
{
    // Cleaned up (nothing to do)
}

void ClientHandler::fillSessionExtraInfo(json &jVars)
{
    jVars["maxAge"] = 0;
    if ( isSessionActive() )
    {
        jVars["maxAge"] = m_JWTToken.getExpirationTime() - time(nullptr);
    }
}

bool ClientHandler::doesSessionVariableExist(const string &varName)
{
    if ( isSessionActive() )
    {
        return m_JWTToken.hasClaim(varName);
    }
    return false;
}

json ClientHandler::getSessionVariableValue(const string &varName)
{
    if ( isSessionActive() )
    {
        return m_JWTToken.getClaim(varName);
    }
    return {};
}

bool ClientHandler::isSessionActive()
{
    return ( m_JWTHeaderTokenVerified || m_JWTCookieTokenVerified );
}

set<string> ClientHandler::getSessionScopes()
{
    if ( isSessionActive() )
    {
        return m_JWTToken.getAllScopes();
    }
    return {};
}

set<string> ClientHandler::getSessionRoles()
{
    if ( isSessionActive() )
    {
        return m_JWTToken.getAllRoles();
    }
    return {};
}
