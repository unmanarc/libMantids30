#include "clienthandler.h"
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include "json/value.h"
#include <Mantids30/API_RESTful/methodshandler.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <Mantids30/Protocol_HTTP/rsp_cookies.h>
#include <Mantids30/Helpers/encoders.h>

#include <json/config.h>
#include <memory>
#include <string>

#include <string>
#include <json/json.h>

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols::HTTP;

using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;


ClientHandler::ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock) : Servers::Web::APIClientHandler(parent,sock)
{
}


Network::Protocols::HTTP::Status::eRetCode ClientHandler::sessionStart()
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
        m_JWTHeaderTokenVerified = verifyToken(headerBearerToken);
    }
    else if (!cookieBearerToken.empty())
    {
        // if not, continue with the cookie.
        m_JWTCookieTokenVerified = verifyToken(cookieBearerToken);
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
    sessionLogout();
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

void ClientHandler::handleAPIRequest(API::APIReturn * apiReturn,
                                     const string & baseApiUrl,
                                     const uint32_t & apiVersion,
                                     const string &methodMode,
                                     const string &methodName,
                                     const Json::Value & pathParameters,
                                     const Json::Value & postParameters
                                               )
{
    set<string> currentPermissions;
    bool authenticated =false;
    API::RESTful::RequestParameters inputParameters;

    inputParameters.jwtSigner       = this->config->jwtSigner;
    inputParameters.jwtValidator    = this->config->jwtValidator;
    inputParameters.clientRequest   = &clientRequest;

    if ( isSessionActive() )
    {
        currentPermissions = m_JWTToken.getAllPermissions();
        inputParameters.jwtToken = &m_JWTToken;
    }

    if (m_methodsHandler.find(apiVersion) == m_methodsHandler.end())
    {
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API Version Not Found / Resource Not found {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        apiReturn->setError( Network::Protocols::HTTP::Status::S_404_NOT_FOUND,"invalid_api_request","Resource Not Found");
        return;
    }


    json x;
    API::RESTful::MethodsHandler::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_JWTCookieTokenVerified;
    securityParameters.haveJWTAuthHeader = m_JWTHeaderTokenVerified;

    //inputParameters.cookies = m_clientRequest.getAllCookies();
    //securityParameters.genCSRFToken = m_clientRequest.headers.getOptionValueStringByName("GenCSRFToken");

    API::RESTful::MethodsHandler::ErrorCodes result = m_methodsHandler[apiVersion]->invokeResource( methodMode,
                                                                                                    methodName,
                                                                                                    inputParameters,
                                                                                                    currentPermissions,
                                                                                                    securityParameters,
                                                                                                    apiReturn
                                                                                                   );

    switch (result)
    {
    case API::RESTful::MethodsHandler::SUCCESS:
        log(eLogLevels::LEVEL_DEBUG, "restAPI", 2048, "Method Executed {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INVALID_METHOD_MODE:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "Invalid Method Mode {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::RESOURCE_NOT_FOUND:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "Method Not Found {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::AUTHENTICATION_REQUIRED:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "Authentication Not Provided {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INSUFFICIENT_PERMISSIONS:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "Insufficient permissions {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INTERNAL_ERROR:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "Internal Error {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        break;
    default:
        log(eLogLevels::LEVEL_ERR, "restAPI", 2048, "Unknown Error {method=%s, mode=%s}", methodName.c_str(), methodMode.c_str());
        apiReturn->setError( Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR,"invalid_api_request","Unknown Error");
        break;
    }
}

bool ClientHandler::isSessionActive()
{
    return ( m_JWTHeaderTokenVerified || m_JWTCookieTokenVerified );
}

set<string> ClientHandler::getSessionPermissions()
{
    if ( isSessionActive() )
    {
        return m_JWTToken.getAllPermissions();
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

// Helper: Set AccessToken and loggedIn cookies
void ClientHandler::setAccessTokenAndLoggedInCookie(const std::string &token, const uint64_t & maxAge)
{
    // Set the access token cookie:
    serverResponse.setSecureCookie("AccessToken", token, maxAge);
    auto accessToken = serverResponse.cookies.getCookieByName("AccessToken");
    if (accessToken)
    {
        accessToken->path = "/";
        Headers::Cookie loggedInCookie;
        loggedInCookie.setExpiration(accessToken->getExpiration());
        loggedInCookie.secure = true;
        loggedInCookie.httpOnly = false;
        loggedInCookie.path = "/";
        loggedInCookie.value = std::to_string(accessToken->getExpiration());

        // Set the loggedIn cookie (JS accessible).
        serverResponse.cookies.addCookieVal("loggedIn", loggedInCookie);

        // If defined in config->useJSTokenCookie, use the JS accessible cookie token:
        setPostLoginTokenCookie(token, maxAge);
    }
}

// Helper: Set impersonator token if allowed
void ClientHandler::setImpersonatorToken(const uint64_t &maxAge)
{
    serverResponse.setSecureCookie("ImpersonatorToken", clientRequest.getCookie("AccessToken"), maxAge);
    serverResponse.cookies.getCookieByName("ImpersonatorToken")->path = "/";
}

// Helper: Get redirect URL from request or default
std::string ClientHandler::getRedirectURL()
{
    std::string redirectURL = clientRequest.getVars(HTTP_VARS_POST)->getStringValue("redirectURI");
    if (redirectURL.empty())
    {
        redirectURL = "/";
    }
    return redirectURL;
}

Status::eRetCode ClientHandler::handleAuthLoginFunction()
{
    // Here we will absorb the JWT... and transform that on a session...

    // Validate origin
    string requestOrigin = clientRequest.getOrigin();
    if ( config->permittedLoginOrigins.find(requestOrigin) == config->permittedLoginOrigins.end() )
    {
        log(LEVEL_SECURITY_ALERT, "restAPI", 2048,
            "Unauthorized login attempt from disallowed origin {origin=%s}", requestOrigin.c_str());
        return HTTP::Status::S_401_UNAUTHORIZED;
    }

    // Security note (design): We trust the redirection to the trusted origin.

    // Handle token from POST
    string postLoginToken = clientRequest.getVars(HTTP_VARS_POST)->getStringValue("accessToken");
    if (postLoginToken.empty())
    {
        // No need to validate the token, we are asuming that the application has a valid token.
        // Redirect to the URL specified by the 'redirectURI' parameter from the authentication provider,
        // or a default homepage if none is provided securely.
        string redirectURL = getRedirectURL();
        return redirectUsingJS(redirectURL);
    }

    // Verify JWT token
    DataFormat::JWT::Token currentJWTToken;
    bool isTokenValid = this->config->jwtValidator->verify(postLoginToken, &currentJWTToken);
    uint64_t maxAge = currentJWTToken.getExpirationTime()>time(nullptr)? currentJWTToken.getExpirationTime()-time(nullptr) : 0;

    if (!isTokenValid)
    {
        log(Program::Logs::LEVEL_SECURITY_ALERT, "restAPI", 2048, "Invalid JWT token");
        return redirectUsingJS(config->redirectLocationOnLoginFailed);
        return HTTP::Status::S_401_UNAUTHORIZED;
    }

    if ( isSessionActive() )
    {
        // Session is enabled:
        if ( currentJWTToken.hasClaim("impersonator") )
        {
            // Impersonation
            if (!this->config->allowImpersonation)
            {
                // Impersonate
                log(LEVEL_SECURITY_ALERT,"restAPI", 65535, "Impersonation not allowed on this application.");
                return HTTP::Status::S_405_METHOD_NOT_ALLOWED;
            }
            else
            {
                // set the access + logged in cookies:
                setAccessTokenAndLoggedInCookie(postLoginToken, maxAge);

                // Set the impersonation token as the old token (pass->)
                setImpersonatorToken(maxAge);
            }
        }
        else
        {
            // Upgrade the token.
            if (m_JWTToken.getSubject()!=currentJWTToken.getSubject() ||  m_JWTToken.getDomain()!=currentJWTToken.getDomain())
            {
                // If the cookie does not match with the current subject/token and not impersonation cookie
                log(LEVEL_SECURITY_ALERT, "restAPI", 2048, "Login override attempt with different user/domain {origin=%s}, you must logout first.", requestOrigin.c_str());
                return HTTP::Status::S_401_UNAUTHORIZED;
            }
            else
            {
                // set the access + logged in cookies:
                setAccessTokenAndLoggedInCookie(postLoginToken, maxAge);
            }
        }
    }
    else
    {
        // No previous session: set the token.

        // set the access + logged in cookies:
        setAccessTokenAndLoggedInCookie(postLoginToken, maxAge);
    }

    // Redirect to the URL specified by the 'redirectURI' parameter from the authentication provider,
    // or a default homepage if none is provided securely.
    string redirectURL = getRedirectURL();
    return redirectUsingJS(redirectURL);
}

/*
    if (config->takeRedirectLocationOnLoginSuccessFromURL)
    {
        string redirectURL = clientRequest.requestLine.urlVars()->getValue("redirect")->toString();

        if (!isURLSafe(redirectURL) || !isRedirectPathSafeForAuth(redirectURL))
        {
            log(LEVEL_SECURITY_ALERT, "restAPI", 2048,
                "Invalid redirect URL {path=%s, origin=%s}", redirectURL.c_str(), requestOrigin.c_str());
        }
        else
        {
            // beware of dangerous URL's please.
            return redirectUsingJS(redirectURL);
        }
    }
    else
    {
        return redirectUsingJS(config->redirectLocationOnLoginSuccess);
    }*/

// TODO: separar opciones de configuracion específicas para monolith
// Bad authentication:

void ClientHandler::setPostLoginTokenCookie(const string &postLoginToken, const uint64_t & maxAge)
{
    if (config->useJSTokenCookie)
    {
        // deliver the information about the new token via a JS readable cookie:
        Headers::Cookie jsAuthTokenCookie;
        jsAuthTokenCookie.secure = true;
        jsAuthTokenCookie.httpOnly = false;
        jsAuthTokenCookie.setExpirationFromNow(maxAge);
        jsAuthTokenCookie.maxAge = maxAge;
        jsAuthTokenCookie.sameSitePolicy = Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;
        jsAuthTokenCookie.value =  postLoginToken;
        serverResponse.setCookie("jsAuthToken", jsAuthTokenCookie);
        // the cookie should be dropped from the website to avoid XSS attacks to retrieve it.
    }
}

Status::eRetCode ClientHandler::handleAuthFunctions(const string &baseApiUrl, const string &authFunctionName)
{
    // Login callback:
    if (authFunctionName == "callback")
    {
        // Login (Pass the JWT)...
        return handleAuthLoginFunction();
    }
    else if (authFunctionName == "login_redirect" && !m_currentSessionInfo.authSession)
    {
        return serverResponse.setRedirectLocation(config->defaultLoginRedirect);
    }
    else if (authFunctionName == "logout" && m_currentSessionInfo.authSession)
    {
        // Logout... (Mark to terminate the session)
        m_destroySession = true;
        return Protocols::HTTP::Status::S_200_OK;
    }
    else
    {
        log(LEVEL_WARN,"restAPI", 65535, "A/404: Authentication Function Not Found.");
        return HTTP::Status::S_404_NOT_FOUND;
    }
}


void ClientHandler::sessionLogout()
{
    // We are working with cookie tokens...
    if (m_JWTCookieTokenVerified)
    {
        // The session is marked to be destroyed.
        if (m_destroySession)
        {
            std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
            jPayloadOutStr->setFormatted(this->config->useFormattedJSONOutput);
            serverResponse.setDataStreamer(jPayloadOutStr);
            serverResponse.setContentType("application/json", true);

            // Take the impersonator token from the cookie (if exist)...
            string cookieImpersonatorToken = clientRequest.getCookie("ImpersonatorToken");

            bool isImpersonation = !cookieImpersonatorToken.empty();

            if (isImpersonation)
            {
                DataFormat::JWT::Token impersonatorJWTToken;
                bool isTokenValid = this->config->jwtValidator->verify(cookieImpersonatorToken, &impersonatorJWTToken);

                if (isTokenValid)
                {
                    // Just swap the ImpersonatorToken to AuthToken
                    serverResponse.addCookieClearSecure("ImpersonatorToken");
                    serverResponse.setSecureCookie( "AccessToken", cookieImpersonatorToken,
                                                     impersonatorJWTToken.getExpirationTime()>time(nullptr)? impersonatorJWTToken.getExpirationTime()-time(nullptr) : 0 );
                    // deliver the information about the new token...
                    json r;
                    r["success"] = true;
                    r["authToken"] = cookieImpersonatorToken;
                    jPayloadOutStr->setValue(r);
                }
                else
                {
                    log(Program::Logs::LEVEL_SECURITY_ALERT, "restAPI", 2048, "Invalid impersonator token cookie");
                    json r;
                    r["success"] = false;
                    jPayloadOutStr->setValue(r);
                }
            }
            else
            {
                // Clear the AUTH token:
                serverResponse.addCookieClearSecure("AccessToken");

                // deliver ok and no further token.
                json r;
                r["success"] = true;
                r["authToken"] = Json::nullValue;
                jPayloadOutStr->setValue(r);
            }
        }
    }
}

