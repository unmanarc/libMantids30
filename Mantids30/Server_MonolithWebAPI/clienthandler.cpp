#include "clienthandler.h"
#include "sessionsmanager.h"

#include <Mantids30/Sessions/session.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <Mantids30/Server_WebCore/htmliengine.h>
#include <Mantids30/API_Monolith/methodshandler.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/b_mmap.h>
#include <Mantids30/Memory/streamablejson.h>

#include <memory>
#include <stdarg.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

#define IMPERSONATOR_SESSIONID_COOKIENAME "impersonatorSessionId"
#define CURRENT_SESSIONID_COOKIENAME "sessionId"

ClientHandler::ClientHandler(void *parent, std::shared_ptr<StreamableObject> sock)
    : Servers::Web::APIClientHandler(parent,sock)
{
}


// This function is called at the beggining.
Status::eRetCode ClientHandler::sessionStart()
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
        m_currentSessionInfo.authSession = m_currentWebSession->getAuthSession();
        m_currentSessionInfo.halfSessionId = RPCLog::truncateSessionId(m_sessionID);
        m_currentSessionInfo.sessionId = m_sessionID;
        m_currentSessionInfo.isImpersonation = !(m_currentSessionInfo.authSession->getImpersonator().empty());

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
    if (m_currentSessionInfo.authSession)
    {
        // Set this cookie to report only to the javascript the remaining session time.
        setJSSessionTimeOutCookie(m_sessionMaxAge);
        setJSSessionHalfIDCookie(m_sessionID);
        serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_sessionID, m_sessionMaxAge);
        m_sessionsManager->releaseSession(m_sessionID);
    }

    // In case of the session logout is activated, destroy the sesion here.
    sessionLogout();
}

void ClientHandler::handleAPIRequest(API::APIReturn * apiReturn,
                                     const string & baseApiUrl,
                                     const uint32_t & apiVersion,
                                      const std::string &methodMode,
                                     const string & methodName,
                                     const Json::Value & pathParameters,
                                     const Json::Value & postParameters)
{
    //json jPayloadIn;
    Mantids30::Helpers::JSONReader2 reader;

    auto it = m_methodsHandlerByAPIVersion.find(apiVersion);
    if (it == m_methodsHandlerByAPIVersion.end())
    {
        // Key does not exist
        log(LEVEL_ERR, "monolithAPI", 2048, "API version %lu does not exist {method=%s}", apiVersion, methodName.c_str());
        // Method not available for this null session..
        apiReturn->setError(HTTP::Status::S_404_NOT_FOUND,"invalid_api_handling","Method Not Found");
        return;
    }

    API::Monolith::MethodsHandler * methodsHandler = m_methodsHandlerByAPIVersion[apiVersion];

    // TODO: upgrade Token (2fa) y/o  token acompañante.
    if (methodsHandler->doesMethodRequireActiveSession(methodName) && !m_currentWebSession)
    {
        log(LEVEL_ERR, "monolithAPI", 2048, "This method requires full authentication / session {method=%s}", methodName.c_str());
        // Method not available for this null session..
        apiReturn->setError(HTTP::Status::S_404_NOT_FOUND,"invalid_api_handling","Method Not Found");
        return;
    }

    json reasons;

    // Validate that the method requirements are satisfied.
    auto i = methodsHandler->validateMethodRequirements(m_currentSessionInfo.authSession, methodName, &reasons);

    switch (i)
    {
    case API::Monolith::MethodsHandler::VALIDATION_OK:
    {
        log(LEVEL_INFO, "monolithAPI", 2048, "Executing Web Method {method=%s}", methodName.c_str());
        log(LEVEL_DEBUG, "monolithAPI", 8192, "Executing Web Method - debugging parameters {method=%s,params=%s}", methodName.c_str(), Mantids30::Helpers::jsonToString(postParameters).c_str());

        auto start = chrono::high_resolution_clock::now();
        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = finish - start;

        switch (methodsHandler->invoke(m_currentSessionInfo.authSession, methodName, postParameters, apiReturn->outputPayload() ))
        {
        case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

            finish = chrono::high_resolution_clock::now();
            elapsed = finish - start;
            log(LEVEL_INFO, "monolithAPI", 2048, "Web Method executed OK {method=%s, elapsedMS=%f}", methodName.c_str(), elapsed.count());
            log(LEVEL_DEBUG, "monolithAPI", 8192, "Web Method executed OK - debugging parameters {method=%s,params=%s}", methodName.c_str(),Mantids30::Helpers::jsonToString(*(apiReturn->outputPayload())).c_str());
            break;
        case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:
            log(LEVEL_ERR, "monolithAPI", 2048, "Web Method not found {method=%s}", methodName.c_str());
            apiReturn->setError( HTTP::Status::S_404_NOT_FOUND,"invalid_api_handling","Method not found.");
            break;
        default:
            log(LEVEL_ERR, "monolithAPI", 2048, "Unknown error during web method execution {method=%s}", methodName.c_str());
            apiReturn->setError( HTTP::Status::S_401_UNAUTHORIZED,"invalid_api_handling","Method unauthorized.");
            break;
        }
    }
    break;
    case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
    {
        // not enough permissions.
        log(LEVEL_ERR, "monolithAPI", 8192, "Not authorized to execute method {method=%s,reasons=%s}", methodName.c_str(), Mantids30::Helpers::jsonToString(reasons).c_str());
        apiReturn->setError( HTTP::Status::S_401_UNAUTHORIZED,"invalid_api_handling","Method unauthorized.");
        apiReturn->setReasons(reasons);

    }
    break;
    case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
    default:
    {
        log(LEVEL_ERR, "monolithAPI", 2048, "Method not found {method=%s}", methodName.c_str());
        // not enough permissions.
        apiReturn->setError( HTTP::Status::S_404_NOT_FOUND,"invalid_api_handling","Method not found.");
    }
    break;
    }

}

/*
Mitigación: Asegúrate de configurar cabeceras Cache-Control: no-store y Pragma: no-cache en las respuestas de endpoints sensibles como logout.

Conclusión
Usar un header personalizado como X-Session-ID para logout es generalmente seguro, pero debe implementarse junto con:

HTTPS obligatorio.
Cookies con SameSite configurado y Secure.
Validaciones de Origin o Referer en el backend.
Medidas contra XSS en la aplicación.
*/

Status::eRetCode ClientHandler::handleAuthFunctions(const string &baseApiUrl, const string &authFunctionName)
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

Status::eRetCode ClientHandler::handleAuthRetrieveInfoFunction()
{
    Protocols::HTTP::Status::eRetCode ret;
    shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = make_shared<Memory::Streams::StreamableJSON>();
    serverResponse.setDataStreamer(jPayloadOutStr);
    serverResponse.setContentType("application/json", true);
    jPayloadOutStr->setFormatted(this->config->useFormattedJSONOutput);
    json x;

    if (m_currentSessionInfo.authSession)
    {
        string effectiveUserName  = m_currentSessionInfo.authSession->getUser();
        x["loggedUser"]["username"] = effectiveUserName;
        x["isImpersonation"] = m_currentSessionInfo.isImpersonation;
        x["impersonator"] = m_currentSessionInfo.authSession->getImpersonator();
        x["permissions"] = m_currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllPermissionsAsJSON();
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

json ClientHandler::handleAPIInfo(const string &baseApiUrl)
{
    json x;
    // Retrieve software information...
    /*
    {
        "softwareName": "MyWebServer",
        "version": "1.3.5",
        "description": "Monolithic web server for handling user sessions",
        "endpoint" : "v1",
        "buildDate": "2024-10-12"
    }*/

    // Obtener la fecha de compilación
    const char* buildDate = __DATE__; // Macro estándar para fecha de compilación
    const char* buildTime = __TIME__; // Macro estándar para hora de compilación

    x["version"] = config->softwareVersion;
    x["softwareName"] = config->softwareName;
    x["description"] = config->softwareDescription;
    x["endpoint"] = baseApiUrl;
    x["buildDate"] = string(buildDate) + " " + buildTime;

    return x;
}

bool ClientHandler::doesSessionVariableExist(const string &varName)
{
    if (m_currentSessionInfo.authSession)
    {       
        return m_currentSessionInfo.authSession->doesSessionVariableExist(varName);
    }
    return false;
}

json ClientHandler::getSessionVariableValue(const string &varName)
{
    if (m_currentSessionInfo.authSession)
    {
        return m_currentSessionInfo.authSession->getSessionVariableValue(varName);
    }
    return {};

}

void ClientHandler::fillSessionExtraInfo(json &jVars)
{
    jVars["maxAge"] = (Json::UInt64) 0;
    if (m_currentSessionInfo.authSession)
    {
        jVars["maxAge"] =(Json::UInt64) m_sessionMaxAge;
    }
}

bool ClientHandler::getIsInActiveSession()
{
    return m_currentSessionInfo.authSession && !(m_currentSessionInfo.authSession->isSessionRevoked());
}

set<string> ClientHandler::getSessionPermissions()
{
    if (m_currentSessionInfo.authSession)
    {
        return m_currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllPermissions();
    }
    return {};
}

set<string> ClientHandler::getSessionRoles()
{
    if (m_currentSessionInfo.authSession)
    {
        return m_currentSessionInfo.authSession->getJWTAuthenticatedInfo().getAllRoles();
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
    Headers::Cookie simpleJSSecureCookie;
    simpleJSSecureCookie.value = "1";
    simpleJSSecureCookie.secure = true;
    simpleJSSecureCookie.httpOnly = false;
    simpleJSSecureCookie.setExpirationFromNow(maxAge);
    simpleJSSecureCookie.maxAge = maxAge;
    simpleJSSecureCookie.sameSitePolicy = Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;
    serverResponse.setCookie("jsSessionTimeout", simpleJSSecureCookie);
}

void ClientHandler::setJSSessionHalfIDCookie( const string & sessionID )
{
    // This cookie is readeable by the javascript code inside the web, so the web will know the half session id.
    // The other half is not known, so the javascript can't substract the session id to exfiltrate the session.
    if (m_currentSessionInfo.authSession)
    {
        Headers::Cookie simpleJSSecureCookie;
        simpleJSSecureCookie.secure = true;
        simpleJSSecureCookie.httpOnly = false;
        simpleJSSecureCookie.setExpirationFromNow(m_sessionMaxAge);
        simpleJSSecureCookie.maxAge = (m_sessionMaxAge);
        simpleJSSecureCookie.sameSitePolicy = Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;
        simpleJSSecureCookie.value = RPCLog::truncateSessionId(sessionID);
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
            serverResponse.addCookieClearSecure("jsSessionTimeout");
            serverResponse.addCookieClearSecure("jsSessionHalfID");
            serverResponse.addCookieClearSecure(CURRENT_SESSIONID_COOKIENAME);
            log(LEVEL_INFO, "monolithAPI", 2048, "Logged Out {sessionId=%s}", RPCLog::truncateSessionId(cookieSessionID).c_str());
        }
        else
        {
            // Impersonation logout:
            setJSSessionTimeOutCookie(parentSessionMaxAge);
            setJSSessionHalfIDCookie(m_impersonatorSessionID);
            // Remove the impersonation cookie:
            serverResponse.addCookieClearSecure(IMPERSONATOR_SESSIONID_COOKIENAME);
            // Fall back to the impersonator cookie.
            serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_impersonatorSessionID, m_sessionMaxAge);
            log(LEVEL_INFO, "monolithAPI", 2048, "Impersonation Logged Out {sessionId=%s}", RPCLog::truncateSessionId(cookieSessionID).c_str());
        }
    }

    // destroy the current session anyway (if defined), by example if the session is not in the manager, when you don't require to destroy it.
    if (!m_sessionID.empty())
        m_sessionsManager->destroySession(m_sessionID);

    // We are at the end, now, bye.
}

// TODO: tambien cambiar en restful.
Status::eRetCode ClientHandler::handleAuthLoginFunction()
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
    string postLoginToken = clientRequest.getVars(HTTP_VARS_POST)->getStringValue("accessToken");
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

        json networkClientInfo = clientRequest.networkClientInfo.getNetworkClientInfo();
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
        serverResponse.setSecureCookie(CURRENT_SESSIONID_COOKIENAME, m_sessionID, m_sessionMaxAge);
        if (isImpersonation)
        {
            serverResponse.setSecureCookie(IMPERSONATOR_SESSIONID_COOKIENAME, impersonatorSessionID, m_sessionMaxAge);
        }

        // Redirect to the URL specified by the 'redirectURI' parameter from the authentication provider,
        // or a default homepage if none is provided securely.
        string redirectURL = clientRequest.getVars(HTTP_VARS_POST)->getStringValue("redirectURI");
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

Status::eRetCode ClientHandler::handleAuthLogoutFunction()
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
Status::eRetCode ClientHandler::handleAuthUpdateLastActivityFunction()
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

bool ClientHandler::validateSessionAntiCSRFMechanism()
{
    string sessionIdFromHeader = clientRequest.getHeaderOption("X-HalfSession-ID");
    string sessionIdFromCookie = clientRequest.getCookie(CURRENT_SESSIONID_COOKIENAME);

    // Validate session header == cookie, in theory, JS does not allow header manipulation via CSRF
    if ( sessionIdFromHeader == RPCLog::truncateSessionId(sessionIdFromCookie)  )
    {
        return true;
    }

    string referer = clientRequest.getHeaderOption("Referer");
    log(LEVEL_SECURITY_ALERT, "monolithAPI", 2048, "Invalid CSRF Validation {path=%s, referer=%s}",
        clientRequest.getURI().c_str(), referer.c_str());

    return false;
}

