#include "clienthandler.h"

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

HTTP::Status::Codes ClientHandler::handleAuthFunctions(const string &baseApiUrl, const string &authFunctionName)
{
    // Login callback:
    if (authFunctionName == "login")
    {
        // Login (Pass the JWT)...
        return handleAuthLoginFunction();
    }
    else if (authFunctionName == "login_redirect" && !m_currentWebSession)
    {
        return serverResponse.setRedirectLocation(config->defaultLoginRedirect);
    }
    else if (authFunctionName == "retrieveInfo" && m_currentWebSession)
    {
        // Retrieve information about the current authentication...
        return handleAuthRetrieveInfoFunction();
    }
    else if (authFunctionName == "updateLastActivity" && m_currentWebSession)
    {
        // Retrieve information about the current authentication...
        return handleAuthUpdateLastActivityFunction();
    }
    else if (authFunctionName == "logout" && m_currentWebSession)
    {
        // Logout... (Mark to terminate the session)
        return handleAuthLogoutFunction();
    }
    else
    {
        log(LEVEL_WARN,"monolithAPI", 65535, "A/404: Authentication Function Not Found.");
        return HTTP::Status::S_404_NOT_FOUND;
    }
}

HTTP::Status::Codes ClientHandler::handleAuthRetrieveInfoFunction()
{
    Protocols::HTTP::Status::Codes ret;
    shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = make_shared<Memory::Streams::StreamableJSON>();
    serverResponse.setDataStreamer(jPayloadOutStr);
    serverResponse.setContentType("application/json", true);
    jPayloadOutStr->setIsFormatted(this->config->useFormattedJSONOutput);
    json x;

    if (isSessionActive())
    {
        string effectiveUserName  = m_currentSessionInfo.authSession->getUser();
        x["loggedUser"]["username"] = effectiveUserName;
        x["isImpersonation"] = m_currentSessionInfo.isImpersonation;
        x["impersonator"] = m_currentSessionInfo.authSession->getImpersonator();
        x["scopes"] = m_currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllScopesAsJSON();
        x["roles"] = m_currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllRolesAsJSON();
        x["currentActivity"]["first"] = (Json::UInt64)m_currentSessionInfo.authSession->getFirstActivity();
        x["currentActivity"]["last"] = (Json::UInt64)m_currentSessionInfo.authSession->getLastActivity();
        x["currentActivity"]["sessionMaxAge"] = (Json::UInt64) m_sessionMaxAge;
        if (m_sessionsManager)
        {
            x["limits"]["maxInactiveTime"] =(Json::UInt64) m_sessionsManager->getMaxInactiveSeconds();
            x["limits"]["maxSessionsPerUser"] = (Json::UInt64) m_sessionsManager->getMaxSessionsPerUser();
            x["loggedUser"]["sessionsInfo"] = m_sessionsManager->getUserSessionsInfo(effectiveUserName);
        }
    }

    jPayloadOutStr->setValue(x);
    ret = HTTP::Status::S_200_OK;

    return ret;
}


HTTP::Status::Codes ClientHandler::handleAuthLoginFunction()
{
    // Here we will absorb the JWT... and transform that on a session...
    string requestOrigin = clientRequest.getOrigin();

    if ( config->permittedLoginOrigins.find(requestOrigin) == config->permittedLoginOrigins.end() )
    {
        log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048,
            "Unauthorized login attempt from disallowed origin {origin=%s}", requestOrigin.c_str());
        return HTTP::Status::S_401_UNAUTHORIZED;
    }

    // TODO: en el futuro permitir header usando un policy y OPTIONS.

    // Check for the authorization bearer token...
    string postLoginToken = clientRequest.getVars(HTTP::VARS_POST)->getStringValue("accessToken");
    bool isJWTHeaderTokenVerified = verifyToken(postLoginToken);

    // The token is OK (authenticated).
    if (isJWTHeaderTokenVerified)
    {
        bool isImpersonation = m_JWTToken.hasClaim("impersonator");
        string impersonatorSessionID;
        string impersonatorUser;
        string impersonatorDomain;

        if (isImpersonation && !(this->config->allowImpersonation))
        {
            // LOG
            log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Impersonation not allowed on this application.");
            return HTTP::Status::S_405_METHOD_NOT_ALLOWED;
        }

        if (!isImpersonation && m_currentWebSession)
        {
            // LOG
            log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Login token override is not allowed. Please logout first.");
            return HTTP::Status::S_403_FORBIDDEN;
        }

        if (m_currentWebSession && isImpersonation)
        {
            impersonatorSessionID = m_currentSessionInfo.sessionId;
            impersonatorUser = m_currentSessionInfo.authSession->getUser();
            impersonatorDomain = m_currentSessionInfo.authSession->getDomain();

            if ( m_currentSessionInfo.authSession )
            {
                string jwtDomain = JSON_ASSTRING_D(m_JWTToken.getClaim("domain"),"");
                string jwtImpersonator = JSON_ASSTRING_D(m_JWTToken.getClaim("impersonator"),"");

                if ( impersonatorUser != jwtImpersonator )
                {
                    // LOG
                    log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Impersonator in token does not match with logged in session.");
                    return HTTP::Status::S_403_FORBIDDEN;
                }

                if (jwtDomain != impersonatorDomain && this->config->allowImpersonationBetweenDifferentDomains)
                {
                    // LOG
                    log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Impersonation is not allowed between different domains.");
                    return HTTP::Status::S_403_FORBIDDEN;
                }
            }
        }

        // TODO: exit impersonation.
        shared_ptr<Sessions::Session> session = make_shared<Sessions::Session>( m_JWTToken );

        json networkClientInfo = clientRequest.networkClientInfo.toJSON();
        networkClientInfo["userAgent"] = clientRequest.userAgent;
        networkClientInfo["startTime"] = (uint64_t)time(nullptr);

        // TODO: migrar esto en libMantids2 a shared pointer.
        m_sessionID = m_sessionsManager->createSession(session, networkClientInfo, &m_sessionMaxAge);
        if (m_sessionID == "")
        {
            // Too many sessions for this user or error inserting the session.
            return HTTP::Status::S_503_SERVICE_UNAVAILABLE;
        }

        // If session id is found, populate user data.
        // Extract and store user-related information from the token.
        m_currentSessionInfo.isImpersonation = isImpersonation;
        m_currentSessionInfo.authSession = session;
        m_currentSessionInfo.halfSessionId = RPCLog::truncateSessionId(m_sessionID);
        m_currentSessionInfo.sessionId = m_sessionID;

        // TODO: log levels ORED...
        if (isImpersonation)
        {
            log(LEVEL_INFO, "monolithAPI", 2048, "Logged in using impersonation token {user=%s, sessionId=%s} by impersonator {user=%s, sessionId=%s}",
                m_currentSessionInfo.authSession->getUser().c_str(),
                RPCLog::truncateSessionId(m_sessionID).c_str(),
                impersonatorUser.c_str(),
                RPCLog::truncateSessionId(impersonatorSessionID).c_str());
        }
        else
        {
            log(LEVEL_INFO, "monolithAPI", 2048, "Logged in using token {user=%s, sessionId=%s}",
                m_currentSessionInfo.authSession->getUser().c_str(),
                RPCLog::truncateSessionId(m_sessionID).c_str());
        }

        // Set the session ID as a session cookie...
        // Same site is enforced as strict, so CSRF should not be available, not accesible via JS
        // However the cookie will be sent when an application lives in the same server...
        // but the application will not be able to know the sessionId to be sent on the header.
        serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_sessionID, m_sessionMaxAge,"/");
        if (isImpersonation)
        {
            serverResponse.setSecureCookie(IMPERSONATOR_SESSIONID_COOKIENAME, impersonatorSessionID, m_sessionMaxAge, "/");
        }

        // Redirect to the URL specified by the 'redirectURI' parameter from the authentication provider,
        // or a default homepage if none is provided securely.
        string redirectURL = clientRequest.getVars(HTTP::VARS_POST)->getStringValue("redirectURI");
        if (redirectURL.empty())
        {
            redirectURL = "/";  // Default to home page
        }
        return redirectUsingJS(redirectURL);

        /*        if (config->takeRedirectLocationOnLoginSuccessFromURL)
        {
            string redirectURL = clientRequest.requestLine.urlVars()->getValue("redirect")->toString();

            if (!isURLSafe(redirectURL) || !isRedirectPathSafeForAuth(redirectURL))
            {
                log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048,
                    "Invalid redirect URL {path=%s, origin=%s}", redirectURL.c_str(), requestOrigin.c_str());
            }
            else
            {
                // beware of dangerous URL's please.
                return serverResponse.setRedirectLocation(redirectURL);
            }
        }
        else
        {
            return serverResponse.setRedirectLocation(config->redirectLocationOnLoginSuccess);
        }*/
    }

    // Bad authentication:
    return serverResponse.setRedirectLocation(config->redirectLocationOnLoginFailed);
}

HTTP::Status::Codes ClientHandler::handleAuthLogoutFunction()
{
    string requestOrigin = clientRequest.getOrigin();

    if (!requestOrigin.empty() || !validateSessionAntiCSRFMechanism())
    {
        log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Failed to logout (external origin or CSRF attempt)");
        return HTTP::Status::S_405_METHOD_NOT_ALLOWED;
    }

    m_destroySession = true;
    return HTTP::Status::S_202_ACCEPTED;
}

// With this API, we can update the last activity.
HTTP::Status::Codes ClientHandler::handleAuthUpdateLastActivityFunction()
{
    string requestOrigin = clientRequest.getOrigin();

    if (!requestOrigin.empty() || !validateSessionAntiCSRFMechanism())
    {
        log(LEVEL_SECURITY_ALERT,"monolithAPI", 65535, "Failed to update the last activity (external origin or CSRF attempt)");
        return HTTP::Status::S_405_METHOD_NOT_ALLOWED;
    }

    if (m_currentWebSession)
    {
        m_currentWebSession->getAuthSession()->updateLastActivity();
    }

    return HTTP::Status::S_202_ACCEPTED;
}

