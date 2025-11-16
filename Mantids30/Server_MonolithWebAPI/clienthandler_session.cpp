#include "clienthandler.h"

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

// This function is called at the beggining.
HTTP::Status::Codes ClientHandler::sessionStart()
{
    m_sessionID = clientRequest.getCookie(CURRENT_SESSIONID_COOKIENAME);
    m_impersonatorSessionID = clientRequest.getCookie(IMPERSONATOR_SESSIONID_COOKIENAME);

    if (!WebSessionsManager::validateSessionIDFormat(m_sessionID))
    {
        log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "Invalid session ID format");
        // No session to load.
        m_sessionID = "";
        m_impersonatorSessionID = "";
        return Protocols::HTTP::Status::S_403_FORBIDDEN;
    }

    if (!WebSessionsManager::validateSessionIDFormat(m_impersonatorSessionID))
    {
        log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "Invalid impersonator session ID format");
        // No session to load.
        m_sessionID = "";
        m_impersonatorSessionID = "";
        return Protocols::HTTP::Status::S_403_FORBIDDEN;
    }

    // Loading session, please validate the Anti-CSRF for sessions before.
    if (!m_sessionID.empty() && !validateSessionAntiCSRFMechanism())
    {
        // Invalid request.
        log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "Anti-CSRF Failed. Header does not match with cookie.");
        return Protocols::HTTP::Status::S_403_FORBIDDEN;
    }

    // update activity on impersonation parent on each access...
    updateActivityOnImpersonatorSession();

    m_currentWebSession = m_sessionsManager->openSession(m_sessionID, &m_sessionMaxAge);
    if (m_currentWebSession)
    {
        // If session id is found, populate user data.
        // Extract and store user-related information from the token.
        currentSessionInfo.authSession = m_currentWebSession->getAuthSession();
        currentSessionInfo.halfSessionId = RPCLog::truncateSessionId(m_sessionID);
        currentSessionInfo.sessionId = m_sessionID;
        currentSessionInfo.isImpersonation = !(currentSessionInfo.authSession->getImpersonator().empty());

        if (!config->allowFloatingClients)
        {
            // check if the IP changed
            if (!m_currentWebSession->compareRemoteAddress( clientRequest.networkClientInfo.REMOTE_ADDR))
            {
                log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "Floating IP not allowed for session {sessionId=%s}",RPCLog::truncateSessionId(m_sessionID).c_str());
                // No session to load.
                m_sessionID = "";
                m_impersonatorSessionID = "";
                return Protocols::HTTP::Status::S_403_FORBIDDEN;
            }
        }
        if (!m_currentWebSession->compareUserAgent(clientRequest.userAgent))
        {
            log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "User Agent not allowed to change for session {sessionId=%s}",RPCLog::truncateSessionId(m_sessionID).c_str());
            // No session to load.
            m_sessionID = "";
            m_impersonatorSessionID = "";
            return Protocols::HTTP::Status::S_403_FORBIDDEN;
        }

    }
    else
    {
        // No session found with this session id, then execute anything without session and then logout.
        m_destroySession = true;
        m_sessionID = ""; // Invalidate this session ID and continue without session.
    }
    return Protocols::HTTP::Status::S_200_OK;
}

// This function is called at the end.
void ClientHandler::sessionCleanup()
{
    // Release the session here:
    if (isSessionActive())
    {
        // Set this cookie to report only to the javascript the remaining session time.
        setJSSessionTimeOutCookie(m_sessionMaxAge);
        setJSSessionHalfIDCookie(m_sessionID);
        serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_sessionID, m_sessionMaxAge, "/");
        m_sessionsManager->releaseSession(m_sessionID);
    }

    // In case of the session logout is activated, destroy the sesion here.
    sessionLogout();
}


bool ClientHandler::doesSessionVariableExist(const string &varName)
{
    if (isSessionActive())
    {
        return currentSessionInfo.authSession->doesSessionVariableExist(varName);
    }
    return false;
}

json ClientHandler::getSessionVariableValue(const string &varName)
{
    if (isSessionActive())
    {
        return currentSessionInfo.authSession->getSessionVariableValue(varName);
    }
    return {};

}

void ClientHandler::fillSessionExtraInfo(json &jVars)
{
    jVars["maxAge"] = (Json::UInt64) 0;
    if (isSessionActive())
    {
        jVars["maxAge"] =(Json::UInt64) m_sessionMaxAge;
    }
}

bool ClientHandler::isSessionActive()
{
    return currentSessionInfo.authSession && !(currentSessionInfo.authSession->isSessionRevoked());
}

set<string> ClientHandler::getSessionScopes()
{
    if (isSessionActive())
    {
        return currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllScopes();
    }
    return {};
}

set<string> ClientHandler::getSessionRoles()
{
    if (isSessionActive())
    {
        return currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllRoles();
    }
    return {};
}

void ClientHandler::updateActivityOnImpersonatorSession()
{
    if ( !m_impersonatorSessionID.empty() )
    {
        uint64_t maxAge;
        WebSession * webSession = m_sessionsManager->openSession(m_impersonatorSessionID, &maxAge);
        if (webSession)
        {
            webSession->getAuthSession()->updateLastActivity();
        }
        m_sessionsManager->releaseSession(m_impersonatorSessionID);
    }
}

void ClientHandler::setJSSessionTimeOutCookie(const uint64_t &maxAge)
{
    // This cookie is readeable by the javascript code inside the web, so the web will know how much time is left to the session
    HTTP::Headers::Cookie simpleJSSecureCookie;
    simpleJSSecureCookie.value = "1";
    simpleJSSecureCookie.secure = true;
    simpleJSSecureCookie.httpOnly = false;
    simpleJSSecureCookie.setExpirationFromNow(maxAge);
    simpleJSSecureCookie.maxAge = maxAge;
    simpleJSSecureCookie.sameSitePolicy = Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;
    simpleJSSecureCookie.path = "/";
    serverResponse.setCookie("jsSessionTimeout", simpleJSSecureCookie);
}

void ClientHandler::setJSSessionHalfIDCookie( const string & sessionID )
{
    // This cookie is readeable by the javascript code inside the web, so the web will know the half session id.
    // The other half is not known, so the javascript can't substract the session id to exfiltrate the session.
    if (isSessionActive())
    {
        HTTP::Headers::Cookie simpleJSSecureCookie;
        simpleJSSecureCookie.secure = true;
        simpleJSSecureCookie.httpOnly = false;
        simpleJSSecureCookie.setExpirationFromNow(m_sessionMaxAge);
        simpleJSSecureCookie.maxAge = (m_sessionMaxAge);
        simpleJSSecureCookie.sameSitePolicy = Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;
        simpleJSSecureCookie.value = RPCLog::truncateSessionId(sessionID);
        simpleJSSecureCookie.path = "/"; // all the site.
        serverResponse.setCookie("jsSessionHalfID", simpleJSSecureCookie);
    }
}

void ClientHandler::sessionLogout()
{
    string cookieSessionID = clientRequest.getCookie(CURRENT_SESSIONID_COOKIENAME);
    bool isImpersonation = false;
    uint64_t parentSessionMaxAge = 0;

    // Invalid session ID, don't do anything with this...
    if (!WebSessionsManager::validateSessionIDFormat(cookieSessionID))
    {
        return;
    }

    // Check if the m_impersonatorSessionID contains the parent session and is currently valid.
    WebSession * parentWebSession = m_sessionsManager->openSession(m_impersonatorSessionID, &parentSessionMaxAge);
    if (parentWebSession)
    {
        // detected impersonation:
        isImpersonation = true;
        m_sessionsManager->releaseSession(m_impersonatorSessionID);
    }

    if (m_destroySession)
    {
        if (!isImpersonation)
        {
            // Not impersonation, normal logout
            serverResponse.addCookieClearSecure("jsSessionTimeout", "/");
            serverResponse.addCookieClearSecure("jsSessionHalfID", "/");
            serverResponse.addCookieClearSecure(CURRENT_SESSIONID_COOKIENAME, "/");
            log(LEVEL_INFO, "monolithAPI", 2048, "Logged Out {sessionId=%s}", RPCLog::truncateSessionId(cookieSessionID).c_str());
        }
        else
        {
            // Impersonation logout:
            setJSSessionTimeOutCookie(parentSessionMaxAge);
            setJSSessionHalfIDCookie(m_impersonatorSessionID);
            // Remove the impersonation cookie:
            serverResponse.addCookieClearSecure(IMPERSONATOR_SESSIONID_COOKIENAME,"/");
            // Fall back to the impersonator cookie.
            serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_impersonatorSessionID, m_sessionMaxAge,"/");
            log(LEVEL_INFO, "monolithAPI", 2048, "Impersonation Logged Out {sessionId=%s}", RPCLog::truncateSessionId(cookieSessionID).c_str());
        }
    }

    // destroy the current session anyway (if defined), by example if the session is not in the manager, when you don't require to destroy it.
    if (!m_sessionID.empty())
        m_sessionsManager->destroySession(m_sessionID);

    // We are at the end, now, bye.
}
