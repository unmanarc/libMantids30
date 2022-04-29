#include "webclienthandler.h"

#include <boost/algorithm/string/predicate.hpp>
#include <mdz_mem_vars/b_mmap.h>
#include <mdz_xrpc_common/json_streamable.h>
#include <mdz_xrpc_common/retcodes.h>
#include <mdz_hlp_functions/crypto.h>
#include <mdz_hlp_functions/json.h>

#include <stdarg.h>
#include <fstream>
#include <streambuf>
#include <boost/algorithm/string/replace.hpp>

#ifdef _WIN32
#include <stdlib.h>
// TODO: check if _fullpath mitigate transversal.
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

using namespace Mantids::Application::Logs;
using namespace Mantids::Network::HTTP;
using namespace Mantids::Memory;
using namespace Mantids::RPC::Web;
using namespace Mantids::RPC;
using namespace Mantids;
using namespace std;

WebClientHandler::WebClientHandler(void *parent, Memory::Streams::Streamable *sock) : HTTPv1_Server(sock)
{
    // TODO: rpc logs?

    rpcLog = nullptr;
    methodsManager = nullptr;
    authDomains = nullptr;
    sessionsManager = nullptr;
    resourceFilter = nullptr;
}

WebClientHandler::~WebClientHandler()
{
}

void WebClientHandler::setAuthenticators(Mantids::Authentication::Domains *authenticator)
{
    authDomains = authenticator;
}

Response::StatusCode WebClientHandler::processClientRequest()
{
    // RPC MODE:
    sLocalRequestedFileInfo fileInfo;
    Response::StatusCode ret  = Response::StatusCode::S_404_NOT_FOUND;
    if (getRequestURI() == "/api") return processRPCRequest();

    // WEB MODE:
    std::string sSessionId = getRequestCookie("sessionId");
    uint64_t uMaxAge=0;
    WebSession * hSession = sessionsManager->openSession(sSessionId, &uMaxAge);

    if ( //staticContent ||
         (getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, ".html") ||
          getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, "index.html") ||
          getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, "/index.html") ||
          getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, "")
          ) && !fileInfo.isDir
         )
    {
        // Evaluate...
        sFilterEvaluation e;

        Mantids::Authentication::Manager * authorizer = hSession?authDomains->openDomain(hSession->authSession->getAuthDomain()) : nullptr;

        if ( resourceFilter )
            e = resourceFilter->evaluateAction(fileInfo.sRealRelativePath, !hSession?nullptr:hSession->authSession, authorizer);

        if (e.accept)
        {
            if (e.redirectLocation.empty())
                ret = Response::StatusCode::S_200_OK;
            else
                ret = setResponseRedirect( e.redirectLocation );
        }
        else
            ret = Response::StatusCode::S_403_FORBIDDEN;

        log(LEVEL_DEBUG,hSession, "fileServer", 2048, "R/ - LOCAL - %03d: %s",Response::Status::getHTTPStatusCodeTranslation(ret),fileInfo.sRealFullPath.c_str());
    }
    else
    {
        log(LEVEL_WARN,hSession, "fileServer", 65535, "R/404: %s",getRequestURI().c_str());
    }

    if (ret != Response::StatusCode::S_200_OK)
    {
        // Stream nothing....
        setResponseDataStreamer(nullptr,false);
    }

    if ( useHTMLIEngine &&
         getContentType() == "text/html" ) // The content type has changed during the map.
        processHTMLIEngine(fileInfo.sRealFullPath,hSession,uMaxAge);

    if (hSession)
        sessionsManager->closeSession(sSessionId);

    if (ret==Response::StatusCode::S_404_NOT_FOUND && !redirectOn404.empty())
    {
        ret = setResponseRedirect( redirectOn404 );
    }

    log(ret==Response::StatusCode::S_200_OK?LEVEL_INFO:LEVEL_WARN,hSession,
        "fileServer", 2048, "R/%03d: %s",
        Response::Status::getHTTPStatusCodeTranslation(ret),
        ret==Response::StatusCode::S_200_OK?fileInfo.sRealRelativePath.c_str():getRequestURI().c_str());


    return ret;
}

// TODO: documentar los privilegios cargados de un usuario

Response::StatusCode WebClientHandler::processHTMLIEngine( const std::string & sRealFullPath,WebSession * hSession, uint64_t uMaxAge )
{
    // Drop the MMAP container:

    std::string fileContent;

    if (boost::starts_with(sRealFullPath,"MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids::Memory::Containers::B_MEM *)getResponseDataStreamer())->toString();
        setResponseDataStreamer(nullptr,false);
    }
    else
    {
        setResponseDataStreamer(nullptr,false);
        // Local resource.
        std::ifstream fileStream(sRealFullPath);
        if (!fileStream.is_open())
        {
            log(LEVEL_ERR,hSession, "fileServer", 2048, "file not found: %s",sRealFullPath.c_str());
            return Response::StatusCode::S_404_NOT_FOUND;
        }
        // Pass the file to a string.
        fileContent = std::string((std::istreambuf_iterator<char>(fileStream)),std::istreambuf_iterator<char>());
        fileStream.close();
    }


    // PRECOMPILE _STATIC_TEXT
    boost::match_flag_type flags = boost::match_default;

    // CINC PROCESSOR:
    boost::regex exStaticText("<CINC_(?<TAGOPEN>[^>]*)>(?<INCPATH>[^<]+)<\\/CINC_(?<TAGCLOSE>[^>]*)>",boost::regex::icase);
    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticText, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string tagOpen      = string(whatStaticText[1].first, whatStaticText[1].second);
        string includePath  = string(whatStaticText[2].first, whatStaticText[2].second);
        string tagClose     = string(whatStaticText[3].first, whatStaticText[3].second);

        // GET THE TAG DATA HERE...

        // The path is relative to resourcesLocalPath (beware: admits transversal)
        std::ifstream fileIncludeStream(resourcesLocalPath + includePath);

        if (fileIncludeStream.is_open())
        {
            std::string includeFileContent((std::istreambuf_iterator<char>(fileIncludeStream)),std::istreambuf_iterator<char>());
            if (tagOpen.empty())
                boost::replace_all(fileContent,fulltag, includeFileContent);
            else
                boost::replace_all(fileContent,fulltag, "<" + tagOpen + ">" + includeFileContent + "</" + tagClose + ">" );
        }
        else
        {
            boost::replace_all(fileContent,fulltag, "<!-- HTMLI ENGINE ERROR (FILE NOT FOUND): " + includePath + " -->");

            log(LEVEL_ERR,hSession, "fileserver", 2048, "file not found: %s",sRealFullPath.c_str());
        }
    }

    // CINPUTVAR PROCESSOR:
    boost::regex exStaticTextInputVar("<CINPUTVAR>(?<VARNAME>[^<]+)<\\/CINPUTVAR>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticTextInputVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string varName      = string(whatStaticText[1].first, whatStaticText[1].second);

        // The path is relative to resourcesLocalPath (beware: admits transversal)

        std::string varValue = "";
        if (varName == "csrfToken" && hSession)
            varValue = hSession->sCSRFToken;
        else if (varName == "csrfToken")
        {}
        else if (varName == "softwareVersion")
            varValue = softwareVersion;
        else if (varName == "user" && hSession && hSession->authSession)
            varValue = hSession->authSession->getUserDomainPair().first;
        else if (varName == "user")
        {}
        else if (varName == "domain" && hSession && hSession->authSession)
            varValue = hSession->authSession->getUserDomainPair().second;
        else if (varName == "domain")
        {}
        else if (varName == "maxAge" && hSession )
            varValue = std::to_string(uMaxAge);
        else if (varName == "maxAge")
        {}
        else
        {
            // TODO: when include other vars, sanitize first (maybe via json?)

            log(LEVEL_ERR,hSession, "fileserver", 2048, "var not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
        }

        std::string htmlVar = "<input type=\"hidden\" id=\"" + varName +"\" value=\"" + varValue + "\" />";
        boost::replace_all(fileContent,fulltag, htmlVar);
    }


    // Update last activity on each page load.
    if (hSession && hSession->authSession)
        hSession->authSession->updateLastActivity();

    // Stream the generated content...
    getResponseDataStreamer()->writeString(fileContent);
    return Response::StatusCode::S_200_OK;
}

Response::StatusCode WebClientHandler::processRPCRequest()
{
    bool bDestroySession = false, bCloseSessionHandler = false;
    std::string sSessionId, sRPCMode, sCSRFToken;
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
    MultiAuths extraCredentials;
    Authentication credentials;

    // COOKIES:
    sSessionId = getRequestCookie("sessionId");
    // TODO: filter invalid session id formats.

    // GET VARS:
    sRPCMode = getRequestVars(HTTP_VARS_GET)->getStringValue("mode");

    // HEADERS:
    sCSRFToken = getClientHeaderOption("CSRFToken");

    // POST VARS / EXTRA AUTHS:
    if (!extraCredentials.setAuthentications(getRequestVars(HTTP_VARS_POST)->getStringValue("extraAuth")))
        return Response::StatusCode::S_400_BAD_REQUEST;

    // POST VARS / AUTH:
    if (!credentials.fromString(getRequestVars(HTTP_VARS_POST)->getStringValue("auth")))
        return Response::StatusCode::S_400_BAD_REQUEST;

    if (sSessionId.empty() && usingCSRFToken && sRPCMode == "AUTHCSRF")
        sSessionId = getRequestVars(HTTP_VARS_POST)->getStringValue("sessionId");

    // TODO: active check of origin to avoid cross-domain

    /////////////////////////////////////////////////////////////////
    // OPEN THE SESSION HERE:
    uint64_t uMaxAge;
    WebSession * webSession = sessionsManager->openSession(sSessionId, &uMaxAge);
    if (webSession)
        bCloseSessionHandler = true;
    else
    {
        if (sSessionId!="")
        {
            log(LEVEL_WARN, nullptr, "rpcServer", 2048, "requested session not found {sessionId=%s,mode=%s}",RPCLog::truncateSessionId(sSessionId).c_str(),sRPCMode.c_str());

            addCookieClearSecure("sessionId");
            return Response::StatusCode::S_404_NOT_FOUND;
        }
        sSessionId = ""; // INVALID SESSION ID.
    }

    /////////////////////////////////////////////////////////////////
    // CHECK CSRF TOKEN HERE.
    // TODO: 1 pre-session per user ;-)
    bool csrfValidationOK = true;
    if (usingCSRFToken)
    {
        csrfValidationOK = false;
        if (webSession)
        {
            // The login token has not been confirmed yet...
            // AUTHCONFIRM will confirm this token against csrfToken POST data.
            if (!webSession->bAuthTokenConfirmed && sRPCMode == "AUTHCSRF")
            {
                if (webSession->confirmAuthCSRFToken(sCSRFToken))
                {
                    // Now this method will fixate the introduced session in the browser...
                    log(LEVEL_DEBUG, webSession, "rpcServer", 2048, "CSRF Confirmation Token OK");
                    setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
                    eHTTPResponseCode = Response::StatusCode::S_200_OK;
                }
                else
                {
                    log(LEVEL_ERR, webSession, "rpcServer", 2048, "Invalid CSRF Confirmation Token {mode=%s}", sRPCMode.c_str());
                    eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
                }
            }
            // Session found and auth token already confirmed, CSRF token must match session.
            else if (webSession->bAuthTokenConfirmed && webSession->validateCSRFToken(sCSRFToken))
            {
                log(LEVEL_DEBUG, webSession, "rpcServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (webSession->bAuthTokenConfirmed && !webSession->validateCSRFToken(sCSRFToken) && !(sRPCMode == "CSRFTOKEN" || sRPCMode == "VERSION"))
            {
                log(LEVEL_ERR, webSession, "rpcServer", 2048, "Invalid CSRF Token {mode=%s}", sRPCMode.c_str());
            }
            // We are just going to obtain the CSRF Token
            else if (webSession->bAuthTokenConfirmed && (sRPCMode == "CSRFTOKEN" || sRPCMode == "VERSION"))
                csrfValidationOK = true;
        }
        else if ( sRPCMode == "LOGIN" )
        {
            // Session not found... Allow LOGIN without CSRF validation...
            csrfValidationOK = true;
        }
    }

    if (csrfValidationOK)
    {
        // TODO: change expired pass function.
        /////////////////////////////////////////////////////////////////
        // INITIAL PERSISTENT AUTHENTICATION...
        if ( !webSession && sRPCMode == "LOGIN" && (!usingCSRFToken || sCSRFToken == "00112233445566778899"))
            eHTTPResponseCode = processRPCRequest_INITAUTH(credentials, sSessionId);
        /////////////////////////////////////////////////////////////////
        // POST PERSISTENT AUTHENTICATION...
        else if (webSession && webSession->bAuthTokenConfirmed && sRPCMode == "POSTLOGIN")
            eHTTPResponseCode = processRPCRequest_POSTAUTH(credentials, webSession, &bDestroySession);
        /////////////////////////////////////////////////////////////////
        // CHANGE MY ACCOUNT PASSWORD...
        else if (webSession && // exist a websession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 webSession->authSession->getIsFullyLoggedIn(Mantids::Authentication::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sRPCMode == "CHPASSWD") // CHPASSWD command
            eHTTPResponseCode = processRPCRequest_CHPASSWD(credentials,webSession, &bDestroySession);
        /////////////////////////////////////////////////////////////////
        // TEST MY ACCOUNT PASSWORD...
        else if (webSession && // exist a websession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 webSession->authSession->getIsFullyLoggedIn(Mantids::Authentication::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sRPCMode == "TESTPASSWD") // CHPASSWD command
            eHTTPResponseCode = processRPCRequest_TESTPASSWD(credentials,webSession, &bDestroySession);
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        // GET MY ACCOUNT PASSWORD LIST/DESCRIPTION...
        else if (webSession && // exist a websession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 webSession->authSession->getIsFullyLoggedIn(Mantids::Authentication::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sRPCMode == "PASSWDLIST") // CHPASSWD command
            eHTTPResponseCode = processRPCRequest_PASSWDLIST(webSession);
        /////////////////////////////////////////////////////////////////
        // CSRF TOKEN REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( webSession && webSession->bAuthTokenConfirmed && sRPCMode == "CSRFTOKEN" )
            eHTTPResponseCode = processRPCRequest_CSRFTOKEN(webSession);
        /////////////////////////////////////////////////////////////////
        // AUTH INFO REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( webSession && webSession->bAuthTokenConfirmed && sRPCMode == "AUTHINFO" )
            eHTTPResponseCode = processRPCRequest_AUTHINFO(webSession,uMaxAge);
        /////////////////////////////////////////////////////////////////
        // METHOD EXECUTION...
        else if ( sRPCMode == "EXEC" )
            eHTTPResponseCode = processRPCRequest_EXEC(webSession,&extraCredentials);
        /////////////////////////////////////////////////////////////////
        // PERSISTENT SESSION LOGOUT
        else if ( webSession && sRPCMode == "LOGOUT" )
        {
            eHTTPResponseCode = Response::StatusCode::S_200_OK;
            bDestroySession = true;
        }
    }

    // csrf not required here:
    if ( sRPCMode == "VERSION" )
        eHTTPResponseCode = processRPCRequest_VERSION();

    /////////////////////////////////////////////////////////////////
    // CLEAN UPS...
    // Close the openned session.

    if (bDestroySession && webSession)
    {
        log(LEVEL_INFO, webSession, "rpcServer", 2048, "Closing session");
    }

    if (bCloseSessionHandler)
    {
        // Set this cookie to report only to the javascript the remaining session time.
        Headers::Cookie simpleJSSecureCookie;
        simpleJSSecureCookie.setValue("1");
        simpleJSSecureCookie.setSecure(true);
        simpleJSSecureCookie.setHttpOnly(false);
        simpleJSSecureCookie.setExpirationInSeconds(uMaxAge);
        simpleJSSecureCookie.setMaxAge(uMaxAge);
        simpleJSSecureCookie.setSameSite( Headers::HTTP_COOKIE_SAMESITE_STRICT);
        setResponseCookie("jsSessionTimeout",simpleJSSecureCookie);

        setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
        setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
        sessionsManager->closeSession(sSessionId);
    }
    if (bDestroySession)
    {
        addCookieClearSecure("jsSessionTimeout");
        addCookieClearSecure("sessionId");
        log(LEVEL_DEBUG, nullptr, "rpcServer", 2048, "Destroying session {sessionId=%s,mode=%s}",RPCLog::truncateSessionId(sSessionId).c_str(),sRPCMode.c_str());
        // TODO: redirect on logout?
        sessionsManager->destroySession(sSessionId);
    }


    // return the HTTP response code.
    return eHTTPResponseCode;
}

Response::StatusCode WebClientHandler::processRPCRequest_VERSION()
{
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["version"]  = softwareVersion;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return Response::StatusCode::S_200_OK;
}

Response::StatusCode WebClientHandler::processRPCRequest_AUTHINFO(WebSession *wSession, const uint32_t & uMaxAge)
{
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    (*(jPayloadOutStr->getValue()))["user"]   = wSession->authSession->getUserDomainPair().first;
    (*(jPayloadOutStr->getValue()))["domain"] = wSession->authSession->getUserDomainPair().second;

    (*(jPayloadOutStr->getValue()))["maxAge"] = uMaxAge;

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return Response::StatusCode::S_200_OK;
}

Response::StatusCode WebClientHandler::processRPCRequest_CSRFTOKEN(WebSession *wSession)
{
    // On Page Load...
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["csrfToken"] = wSession->sCSRFToken;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);

    // Update last activity on each page load.
    if (wSession && wSession->authSession)
        wSession->authSession->updateLastActivity();

    return Response::StatusCode::S_200_OK;
}

Response::StatusCode WebClientHandler::processRPCRequest_INITAUTH(const Authentication & auth, string sSessionId)
{
    Mantids::Authentication::Reason authReason;
    //Mantids::Authentication::Session *authSession = nullptr;
    uint64_t uMaxAge;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;


    std::string user = getRequestVars(HTTP_VARS_POST)->getStringValue("user");
    std::string domain = getRequestVars(HTTP_VARS_POST)->getStringValue("domain");

    // Authenticate...
    sSessionId = persistentAuthentication( user,
                                           domain,
                                           auth,
                                           nullptr, &authReason);


    (*(jPayloadOutStr->getValue()))["txt"] = getReasonText(authReason);
    (*(jPayloadOutStr->getValue()))["val"] = static_cast<Json::UInt>(authReason);

    // Set the parameters once, the first time we see sessionid...
    if (!sSessionId.empty())
    {
        WebSession * currentWebSession = sessionsManager->openSession(sSessionId,&uMaxAge);
        if (currentWebSession)
        {
            //authSession = currentWebSession->authSession;
            if (!usingCSRFToken)
            {
                // If not using CSRF Token, the session id will be fixated in the session cookie...
                setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
            }
            else
            {
                // If using CSRF Token, pass the session id by JSON, because this session should not be fixated in the browser
                (*(jPayloadOutStr->getValue()))["sessionId"] = sSessionId;
                (*(jPayloadOutStr->getValue()))["maxAge"] = (Json::UInt64)uMaxAge;
            }
            // The session is openned, the CSRF token should be confirmed...
            (*(jPayloadOutStr->getValue()))["csrfAuthConfirm"] = currentWebSession->sCSRFAuthConfirmToken;

            (*(jPayloadOutStr->getValue()))["nextPassReq"] = false;
            auto i = currentWebSession->authSession->getNextRequiredLoginIdxs();
            if (i.first != 0xFFFFFFFF)
            {
                // No next login idx.
                (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
                (*(jPayloadOutStr->getValue()))["nextPassReq"]["idx"] = i.first;
                (*(jPayloadOutStr->getValue()))["nextPassReq"]["desc"] = i.second;

                log(LEVEL_INFO, currentWebSession, "rpcServer", 2048, "Logged in, waiting for the next authentication factor {val=%d,txt=%s}",
                    JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }
            else
            {
                log(LEVEL_INFO, currentWebSession, "rpcServer", 2048, "Logged in {val=%d,txt=%s}", JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }

            eHTTPResponseCode = Response::StatusCode::S_200_OK;

            sessionsManager->closeSession(sSessionId);
        }
    }
    else
    {
        // TODO: for better log, remove usage of , in user/domain
        log(LEVEL_WARN, nullptr, "rpcServer", 2048, "Invalid Login Attempt {val=%d,txt=%s,user=%s,domain=%s}",
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0),
            JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""),
            user.c_str(),
            domain.c_str());

    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::StatusCode WebClientHandler::processRPCRequest_POSTAUTH(const Authentication &auth, WebSession *currentWebSession, bool * destroySession)
{
    Mantids::Authentication::Reason authReason;
    Mantids::Authentication::Session *authSession = currentWebSession->authSession;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;

    // Authenticate...
    std::string sSessionId = persistentAuthentication( authSession->getAuthUser(),
                                                       authSession->getAuthDomain(),
                                                       auth,
                                                       authSession, &authReason);

    (*(jPayloadOutStr->getValue()))["txt"] = getReasonText(authReason);
    (*(jPayloadOutStr->getValue()))["val"] = static_cast<Json::UInt>(authReason);

    (*(jPayloadOutStr->getValue()))["nextPassReq"] = false;

    // If the password is authenticated, proceed to report the next required pass:
    if ( IS_PASSWORD_AUTHENTICATED(authReason) )
    {
        auto i = authSession->getNextRequiredLoginIdxs();
        if (i.first != 0xFFFFFFFF)
        {
            // No next login idx.
            (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["idx"] = i.first;
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["desc"] = i.second;

            log(LEVEL_INFO, currentWebSession, "rpcServer", 2048, "Authentication factor (%d) OK, waiting for the next authentication factor {val=%d,txt=%s}", auth.getPassIndex(), i.first, i.second.c_str());
        }
        else
        {
            log(LEVEL_INFO, currentWebSession, "rpcServer", 2048, "Authentication factor (%d) OK, Logged in.", auth.getPassIndex());
        }
        eHTTPResponseCode = Response::StatusCode::S_200_OK;
    }
    else
    {
        log(LEVEL_WARN, currentWebSession, "rpcServer", 2048, "Authentication error on factor #(%d), Logged out {val=%d,txt=%s}",auth.getPassIndex(),
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt","")
            );

        // Mark to Destroy the session if the chpasswd is invalid...
        *destroySession = true;
        eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::StatusCode WebClientHandler::processRPCRequest_EXEC(WebSession * wSession, MultiAuths *extraAuths)
{
    Mantids::Authentication::Session *authSession = nullptr;
    if (wSession) authSession= wSession->authSession;
    //bool bDeleteTemporarySession = false;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
    std::string sMethodName = getRequestVars(HTTP_VARS_GET)->getStringValue("method");
    json jPayloadIn;
    Mantids::Helpers::JSONReader2 reader;

    std::string  userName = getRequestVars(HTTP_VARS_POST)->getStringValue("user");
    std::string domainName = getRequestVars(HTTP_VARS_POST)->getStringValue("domain");

    // If there is a session, overwrite the user/domain inputs...
    if (authSession)
    {
        userName = authSession->getAuthUser();
        domainName = authSession->getAuthDomain();
    }

    std::string payloadStr = getRequestVars(HTTP_VARS_POST)->getStringValue("payload");

    if (!getRequestVars(HTTP_VARS_POST)->getStringValue("payload").empty() && !reader.parse(payloadStr, jPayloadIn))
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Invalid JSON Payload for execution {method=%s}", sMethodName.c_str());
        return Response::StatusCode::S_400_BAD_REQUEST;
    }

    if (!authDomains)
    {
        log(LEVEL_CRITICAL, wSession, "rpcServer", 2048, "No authentication domain list exist.");
        return Response::StatusCode::S_500_INTERNAL_SERVER_ERROR;
    }

    if (methodsManager->getMethodRequireFullSession(sMethodName) && !authSession)
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "This method requires full authentication {method=%s}", sMethodName.c_str());
        // Method not available for this null session..
        return Response::StatusCode::S_404_NOT_FOUND;
    }

    // TODO: what happens if we are given with unhandled but valid auths that should not be validated...?
    // Get/Pass the temporary authentications for null and not-null sessions:
    std::set<uint32_t> extraTmpIndexes;
    for (const uint32_t & passIdx : extraAuths->getAuthenticationsIdxs())
    {
        Mantids::Authentication::Reason authReason=temporaryAuthentication( userName,
                                                                            domainName,
                                                                            extraAuths->getAuthentication(passIdx) );

        // Include the pass idx in the Extra TMP Index.
        if ( Mantids::Authentication::IS_PASSWORD_AUTHENTICATED( authReason ) )
        {
            log(LEVEL_INFO, wSession, "rpcServer", 2048, "Adding valid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids::Authentication::getReasonText(authReason));
            extraTmpIndexes.insert(passIdx);
        }
        else
        {
            log(LEVEL_WARN, wSession, "rpcServer", 2048, "Rejecting invalid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids::Authentication::getReasonText(authReason));
        }
    }

    auto authorizer = authDomains->openDomain(domainName);
    if (authorizer)
    {
        json reasons;

        // Validate that the RPC method is ready to go (fully authorized and no password is expired).
        auto i = methodsManager->validateRPCMethodPerms( authorizer,  authSession, sMethodName, extraTmpIndexes, &reasons );

        authDomains->closeDomain(domainName);

        switch (i)
        {
        case VALIDATION_OK:
        {
            if (authSession)
                authSession->updateLastActivity();

            log(LEVEL_INFO, wSession, "rpcServer", 2048, "Executing Web Method {method=%s}", sMethodName.c_str());
            log(LEVEL_DEBUG, wSession, "rpcServer", 8192, "Executing Web Method - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(jPayloadIn).c_str());

            switch (methodsManager->runRPCMethod(authDomains,domainName, authSession, sMethodName, jPayloadIn, jPayloadOutStr->getValue()))
            {
            case Mantids::RPC::METHOD_RET_CODE_SUCCESS:
                log(LEVEL_INFO, wSession, "rpcServer", 2048, "Web Method executed OK {method=%s}", sMethodName.c_str());
                log(LEVEL_DEBUG, wSession, "rpcServer", 8192, "Web Method executed OK - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(jPayloadOutStr->getValue()).c_str());

                eHTTPResponseCode = Response::StatusCode::S_200_OK;
                break;
            case Mantids::RPC::METHOD_RET_CODE_METHODNOTFOUND:
                log(LEVEL_ERR, wSession, "rpcServer", 2048, "Web Method not found {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
                break;
            case Mantids::RPC::METHOD_RET_CODE_INVALIDDOMAIN:
                // This code should never be executed... <
                log(LEVEL_ERR, wSession, "rpcServer", 2048, "Domain not found during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
                break;
            default:
                log(LEVEL_ERR, wSession, "rpcServer", 2048, "Unknown error during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
                break;
            }
        }break;
        case VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
            log(LEVEL_ERR, wSession, "rpcServer", 8192, "Not authorized to execute method {method=%s,reasons=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(reasons).c_str());
            eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
        }break;
        case VALIDATION_METHODNOTFOUND:
        default:
        {
            log(LEVEL_ERR, wSession, "rpcServer", 2048, "Method not found {method=%s}", sMethodName.c_str());
            // not enough permissions.
            eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
        }break;
        }
    }
    else
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Domain not found {method=%s}", sMethodName.c_str());

        // Domain Not found.
        eHTTPResponseCode = Response::StatusCode::S_404_NOT_FOUND;
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::StatusCode WebClientHandler::processRPCRequest_CHPASSWD(const Authentication &oldAuth, WebSession *wSession, bool * destroySession)
{
    Mantids::Authentication::Session *authSession = wSession->authSession;
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    Authentication newAuth;

    // POST VARS / AUTH:
    if (!newAuth.fromString(getRequestVars(HTTP_VARS_POST)->getStringValue("newAuth")))
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Invalid JSON Parsing for new credentials item");
        return Response::StatusCode::S_400_BAD_REQUEST;
    }

    if (oldAuth.getPassIndex()!=newAuth.getPassIndex())
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Provided credential index differs from new credential index.");
        return Response::StatusCode::S_400_BAD_REQUEST;
    }

    uint32_t credIdx = newAuth.getPassIndex();

    auto domainAuthenticator = authDomains->openDomain(authSession->getAuthDomain());
    if (domainAuthenticator)
    {
        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = remoteIP;
        clientDetails.sTLSCommonName = remoteTLSCN;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        auto authReason = domainAuthenticator->authenticate(appName,clientDetails,authSession->getAuthUser(),oldAuth.getPassword(),credIdx);

        if (IS_PASSWORD_AUTHENTICATED(authReason))
        {
            // TODO: alternative/configurable password storage...
            // TODO: check password policy.
            Mantids::Authentication::Secret newSecretData = Mantids::Authentication::createNewSecret(newAuth.getPassword(),Mantids::Authentication::FN_SSHA256);

            (*(jPayloadOutStr->getValue()))["ok"] = domainAuthenticator->accountChangeAuthenticatedSecret(appName,
                                                                                                          authSession->getAuthUser(),
                                                                                                          credIdx,
                                                                                                          oldAuth.getPassword(),
                                                                                                          newSecretData,
                                                                                                          clientDetails
                                                                                                          );

            if ( JSON_ASBOOL((*(jPayloadOutStr->getValue())),"ok",false) == true)
                log(LEVEL_INFO, wSession, "rpcServer", 2048, "Password change requested {idx=%d,result=1}",credIdx);
            else
                log(LEVEL_ERR, wSession, "rpcServer", 2048, "Password change failed due to internal error {idx=%d,result=0}",credIdx);

            eHTTPResponseCode = Response::StatusCode::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, wSession, "rpcServer", 2048, "Password change failed, bad incomming credentials {idx=%d,reason=%s}",credIdx,Mantids::Authentication::getReasonText(authReason));

            // Mark to Destroy the session if the chpasswd is invalid...
            *destroySession = true;
            eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
        }


        authDomains->closeDomain(authSession->getAuthDomain());
    }
    else
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Password change failed, domain authenticator not found {idx=%d}",credIdx);
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;

}

Response::StatusCode WebClientHandler::processRPCRequest_TESTPASSWD(const Authentication &auth, WebSession *wSession, bool * destroySession)
{
    Mantids::Authentication::Session *authSession = wSession->authSession;
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    auto domainAuthenticator = authDomains->openDomain(authSession->getAuthDomain());
    if (domainAuthenticator)
    {
        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = remoteIP;
        clientDetails.sTLSCommonName = remoteTLSCN;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        auto authReason = domainAuthenticator->authenticate(appName,clientDetails,authSession->getAuthUser(),auth.getPassword(),0);
        if (IS_PASSWORD_AUTHENTICATED(authReason))
        {
            log(LEVEL_INFO, wSession, "rpcServer", 2048, "Password validation requested {idx=%d,result=1}",auth.getPassIndex());
            //(*(jPayloadOutStr->getValue()))["ok"] = true;
            eHTTPResponseCode = Response::StatusCode::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, wSession, "rpcServer", 2048, "Password validation failed, bad incomming credentials {idx=%d,reason=%s}",auth.getPassIndex(),Mantids::Authentication::getReasonText(authReason));

            // Mark to Destroy the session if the chpasswd is invalid...
            *destroySession = true;
            eHTTPResponseCode = Response::StatusCode::S_401_UNAUTHORIZED;
        }
        (*(jPayloadOutStr->getValue()))["ok"] = true;


        authDomains->closeDomain(authSession->getAuthDomain());
    }
    else
    {
        log(LEVEL_ERR, wSession, "rpcServer", 2048, "Password validation failed, domain authenticator not found {idx=%d}",auth.getPassIndex());
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::StatusCode WebClientHandler::processRPCRequest_PASSWDLIST(WebSession *wSession)
{
    Mantids::Authentication::Session *authSession = wSession->authSession;
    Response::StatusCode eHTTPResponseCode = Response::StatusCode::S_200_OK;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    auto domainAuthenticator = authDomains->openDomain(authSession->getAuthDomain());
    if (domainAuthenticator)
    {
        std::map<uint32_t, Mantids::Authentication::Secret_PublicData> publics = domainAuthenticator->getAccountAllSecretsPublicData(authSession->getAuthUser());

        uint32_t ix=0;
        for (const auto & i : publics)
        {
            (*(jPayloadOutStr->getValue()))[ix]["badAtttempts"] = i.second.badAttempts;
            (*(jPayloadOutStr->getValue()))[ix]["forceExpiration"] = i.second.forceExpiration;
            (*(jPayloadOutStr->getValue()))[ix]["nul"] = i.second.nul;
            (*(jPayloadOutStr->getValue()))[ix]["passwordFunction"] = i.second.passwordFunction;
            (*(jPayloadOutStr->getValue()))[ix]["expiration"] = (Json::UInt64)i.second.expiration;
            (*(jPayloadOutStr->getValue()))[ix]["description"] = i.second.description;
            (*(jPayloadOutStr->getValue()))[ix]["isExpired"] = i.second.isExpired();
            (*(jPayloadOutStr->getValue()))[ix]["isRequiredAtLogin"] = i.second.requiredAtLogin;
            (*(jPayloadOutStr->getValue()))[ix]["isLocked"] = i.second.locked;

            (*(jPayloadOutStr->getValue()))[ix]["idx"] = i.first;

            ix++;
        }
    }
    else
        eHTTPResponseCode = Response::StatusCode::S_500_INTERNAL_SERVER_ERROR;

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

void WebClientHandler::setSessionsManagger(SessionsManager *value)
{
    sessionsManager = value;
}

void WebClientHandler::setRemoteIP(const std::string &value)
{
    remoteIP = value;
}

std::string WebClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Authentication &authData, Mantids::Authentication::Session *authSession, Mantids::Authentication::Reason * authReason)
{
    json payload;
    std::string sessionId;
    std::map<uint32_t,std::string> stAccountPassIndexesUsedForLogin;

    // Don't allow other than 0 idx in the first auth. (Return empty session ID with internal error.)
    if (!authSession && authData.getPassIndex()!=0)
    {
        *authReason = Mantids::Authentication::REASON_INTERNAL_ERROR;
        return sessionId;
    }

    // Next, if the requested domain is not valid,
    *authReason = Mantids::Authentication::REASON_INVALID_DOMAIN;

    auto domainAuthenticator = authDomains->openDomain(domainName);
    if (domainAuthenticator)
    {

        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = remoteIP;
        clientDetails.sTLSCommonName = remoteTLSCN;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        *authReason = domainAuthenticator->authenticate(appName,clientDetails,userName,authData.getPassword(),authData.getPassIndex(), Mantids::Authentication::MODE_PLAIN,"",&stAccountPassIndexesUsedForLogin);

        authDomains->closeDomain(domainName);
    }

    if ( Mantids::Authentication::IS_PASSWORD_AUTHENTICATED( *authReason ) )
    {
        // If not exist an authenticated session, create a new one.
        if (!authSession)
        {
            authSession = new Mantids::Authentication::Session(appName);
            authSession->setIsPersistentSession(true);
            authSession->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);

            // The first pass/time the list of idx should be filled into.
            if (authData.getPassIndex()==0)
                authSession->setRequiredLoginIdx(stAccountPassIndexesUsedForLogin);

            // Add to session manager (creates web session).
            sessionId = sessionsManager->createWebSession(authSession);

            // Destroy the authentication session if the web session was not inserted.
            if (sessionId == "")
            {
                delete authSession;
            }
        }
        else
        {
            // If exist, just register the current authentication into that session and return the current sessionid
            authSession->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);
            sessionId = authSession->getSessionId();
        }
    }

    return sessionId;
}

Mantids::Authentication::Reason WebClientHandler::temporaryAuthentication( const std::string & userName, const std::string & domainName,const Authentication &authData)
{
    Mantids::Authentication::Reason eReason;

    auto auth = authDomains->openDomain(domainName);
    if (!auth)
        eReason = Mantids::Authentication::REASON_INVALID_DOMAIN;
    else
    {
        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = remoteIP;
        clientDetails.sTLSCommonName = remoteTLSCN;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        eReason = auth->authenticate( appName, clientDetails, userName,authData.getPassword(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->closeDomain(domainName);
    }

    return eReason;
}

string WebClientHandler::getAuthSessionID(Mantids::Authentication::Session *authSession)
{
    return !authSession?"":authSession->getSessionId();
}

void WebClientHandler::log(eLogLevels logSeverity, WebSession *wSession, const std::string & module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);

    if (rpcLog) rpcLog->logVA( logSeverity,
                               remoteIP,
                               !wSession?"" : ( !(wSession->authSession)?"" : wSession->authSession->getSessionId()),
                               !wSession?"" : ( !(wSession->authSession)?"" : wSession->authSession->getAuthUser()),
                               !wSession?"" : ( !(wSession->authSession)?"" : wSession->authSession->getAuthDomain()),
                               module, outSize,fmtLog,args);

    va_end(args);
}

void WebClientHandler::setRedirectOn404(const std::string &newRedirectOn404)
{
    redirectOn404 = newRedirectOn404;
}

void WebClientHandler::setRPCLog(Application::Logs::RPCLog *value)
{
    rpcLog = value;
}


void WebClientHandler::setRemoteTLSCN(const std::string &value)
{
    remoteTLSCN = value;
}

std::string WebClientHandler::getAppName() const
{
    return appName;
}

void WebClientHandler::setAppName(const std::string &value)
{
    appName = value;
}

void WebClientHandler::setUseHTMLIEngine(bool value)
{
    useHTMLIEngine = value;
}

void WebClientHandler::setSoftwareVersion(const std::string &value)
{
    softwareVersion = value;
}

void WebClientHandler::setWebServerName(const std::string &value)
{
    webServerName = value;
    if (!webServerName.empty())
    {
        setResponseServerName(webServerName);
    }
}

void WebClientHandler::setUsingCSRFToken(bool value)
{
    usingCSRFToken = value;
}

void WebClientHandler::setDocumentRootPath(const std::string &value)
{
    char * cFullPath = realpath(value.c_str(), nullptr);
    if (cFullPath)
    {
        resourcesLocalPath = cFullPath;
        free(cFullPath);
    }
    else
        resourcesLocalPath = value;
}

void WebClientHandler::setResourceFilter(ResourcesFilter *value)
{
    resourceFilter = value;
}

void WebClientHandler::setUseFormattedJSONOutput(bool value)
{
    useFormattedJSONOutput = value;
}

void WebClientHandler::setMethodsManager(MethodsManager *value)
{
    methodsManager = value;
}

