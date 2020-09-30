#include "webclienthandler.h"

#include <cx2_mem_vars/b_mmap.h>

#include <fstream>
#include <streambuf>
#include <boost/algorithm/string/replace.hpp>

using namespace CX2::Network::HTTP;
using namespace CX2::Memory;
using namespace CX2::RPC::Web;
using namespace CX2::RPC;
using namespace CX2;
using namespace std;

WebClientHandler::WebClientHandler(void *parent, Memory::Streams::Streamable *sock) : HTTPv1_Server(sock)
{
    // TODO: logs?
}

WebClientHandler::~WebClientHandler()
{
}

void WebClientHandler::setAuthenticators(Authorization::IAuth_Domains *authenticator)
{
    authDomains = authenticator;
}

eHTTP_RetCode WebClientHandler::processClientRequest()
{
    std::string sRealRelativePath, sRealFullPath;
    eHTTP_RetCode ret  = HTTP_RET_404_NOT_FOUND;
    if (getRequestURI() == "/api") return processRPCRequest();

    if (setResponseFileFromURI(resourcesLocalPath, &sRealRelativePath, &sRealFullPath))
    {
        // Evaluate...
        sFilterEvaluation e;
        std::string sSessionId = getRequestCookie("sessionId");
        uint64_t uMaxAge;

        WebSession * hSession = sessionsManager->openSession(sSessionId, &uMaxAge);

        CX2::Authorization::IAuth * authorizer = hSession?authDomains->openDomain(hSession->session->getAuthDomain()) : nullptr;
        if (resourceFilter)  e = resourceFilter->evaluateAction(sRealRelativePath,hSession->session, authorizer);

        if (hSession)
            sessionsManager->closeSession(sSessionId);

        if (e.accept)
        {
            if (e.location.empty())
                ret = HTTP_RET_200_OK;
            else
                ret = setResponseRedirect( e.location );
        }
        else
            ret = HTTP_RET_403_FORBIDDEN;
    }

    if (ret != HTTP_RET_200_OK)
    {
        // Stream nothing....
        setResponseDataStreamer(nullptr,false);
    }

    if ( useHTMLIEngine && getContentType() == "text/html" )
        processHTMLIEngine(sRealFullPath);

    return ret;
}

eHTTP_RetCode WebClientHandler::processHTMLIEngine( const std::string & sRealFullPath )
{
    // Drop the MMAP container:
    setResponseDataStreamer(nullptr,false);
    std::ifstream fileStream(sRealFullPath);
    if (fileStream.is_open())
    {
        // Pass the file to a string.
        std::string fileContent((std::istreambuf_iterator<char>(fileStream)),std::istreambuf_iterator<char>());
        fileStream.close();

        // PRECOMPILE _STATIC_TEXT
        boost::match_flag_type flags = boost::match_default;
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
                boost::replace_all(fileContent,fulltag, "<" + tagOpen + ">" + includeFileContent + "</" + tagClose + ">" );
            }
            else
            {
                boost::replace_all(fileContent,fulltag, "<!-- HTMLI ENGINE ERROR (FILE NOT FOUND): " + includePath + " -->");
            }
        }

        // Stream the generated content...
        getResponseDataStreamer()->writeString(fileContent);
        return HTTP_RET_200_OK;
    }
    return HTTP_RET_404_NOT_FOUND;
}

eHTTP_RetCode WebClientHandler::processRPCRequest()
{
    bool bDestroySession = false, bCloseSessionHandler = false;
    std::string sSessionId, sRPCMode, sCSRFToken;
    eHTTP_RetCode eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
    Request request;

    // COOKIES:
    sSessionId = getRequestCookie("sessionId");

    // GET VARS:
    sRPCMode = getRequestVars(HTTP_VARS_GET)->getStringValue("mode");

    // HEADERS:
    sCSRFToken = getClientHeaderOption("CSRFToken");

    // POST VARS:
    if (!request.setAuthentications(getRequestVars(HTTP_VARS_POST)->getStringValue("auths")))
        return HTTP_RET_400_BAD_REQUEST;

    if (sSessionId.empty() && usingCSRFToken && sRPCMode == "AUTHCSRF")
        sSessionId = getRequestVars(HTTP_VARS_POST)->getStringValue("sessionId");

    // TODO: active check of origin to avoid cross-domain

    /////////////////////////////////////////////////////////////////
    // OPEN THE SESSION HERE:
    uint64_t uMaxAge;
    WebSession * wSession = sessionsManager->openSession(sSessionId, &uMaxAge);
    if (wSession)
        bCloseSessionHandler = true;
    else
    {
        if (sSessionId!="")
        {
            setResponseDeleteSecureCookie("sessionId");
            return HTTP_RET_404_NOT_FOUND;
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
        if (wSession)
        {
            // The login token has not been confirmed yet...
            // AUTHCONFIRM will confirm this token against csrfToken POST data.
            if (!wSession->bAuthTokenConfirmed && sRPCMode == "AUTHCSRF")
            {
                if (wSession->confirmAuthCSRFToken(sCSRFToken))
                {
                    // Now this method will fixate the introduced session in the browser...
                    setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
                    eHTTPResponseCode = HTTP_RET_200_OK;
                }
                else
                    eHTTPResponseCode = HTTP_RET_401_UNAUTHORIZED;
            }
            // Session found and auth token already confirmed, CSRF token must match session.
            else if (wSession->bAuthTokenConfirmed && wSession->validateCSRFToken(sCSRFToken))
                csrfValidationOK = true;
            else if (wSession->bAuthTokenConfirmed && sRPCMode == "CSRFTOKEN")
                csrfValidationOK = true;
        }
        else if ( sRPCMode == "LOGIN" )
        {
            // Session not found... Allow AUTH...
            csrfValidationOK = true;
        }
    }

    if (csrfValidationOK)
    {
        /////////////////////////////////////////////////////////////////
        // PERSISTENT AUTHENTICATION...
        if (!wSession && sRPCMode == "LOGIN" && (!usingCSRFToken || sCSRFToken == "00112233445566778899"))
            eHTTPResponseCode = processRPCRequest_AUTH(&request, sSessionId);
        /////////////////////////////////////////////////////////////////
        // CSRF TOKEN REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( wSession && wSession->bAuthTokenConfirmed && sRPCMode == "CSRFTOKEN" )
            eHTTPResponseCode = processRPCRequest_CSRFTOKEN(wSession);
        /////////////////////////////////////////////////////////////////
        // AUTH INFO REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( wSession && wSession->bAuthTokenConfirmed && sRPCMode == "AUTHINFO" )
            eHTTPResponseCode = processRPCRequest_AUTHINFO(wSession);
        /////////////////////////////////////////////////////////////////
        // METHOD EXECUTION...
        else if ( sRPCMode == "EXEC" )
            eHTTPResponseCode = processRPCRequest_EXEC(wSession->session,&request);
        /////////////////////////////////////////////////////////////////
        // PERSISTENT SESSION LOGOUT
        else if ( wSession && sRPCMode == "LOGOUT" )
        {
            setResponseDeleteSecureCookie("sessionId");
            eHTTPResponseCode = HTTP_RET_200_OK;
            bDestroySession = true;
        }
    }

    // csrf not required here:
    if ( sRPCMode == "VERSION" )
        eHTTPResponseCode = processRPCRequest_VERSION();


    /////////////////////////////////////////////////////////////////
    // CLEAN UPS...
    // Close the openned session.
    if (bCloseSessionHandler)
    {
        setResponseSecureCookie("sessionId", sSessionId, uMaxAge);
        sessionsManager->closeSession(sSessionId);
    }
    if (bDestroySession)
        sessionsManager->destroySession(sSessionId);


    // return the HTTP response code.
    return eHTTPResponseCode;
}

eHTTP_RetCode WebClientHandler::processRPCRequest_VERSION()
{
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["version"]  = softwareVersion;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return HTTP_RET_200_OK;
}

eHTTP_RetCode WebClientHandler::processRPCRequest_AUTHINFO(WebSession *wSession)
{
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["user"]   = wSession->session->getUserDomainPair().first;
    (*(jPayloadOutStr->getValue()))["domain"] = wSession->session->getUserDomainPair().second;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return HTTP_RET_200_OK;
}

eHTTP_RetCode WebClientHandler::processRPCRequest_CSRFTOKEN(WebSession *wSession)
{

    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["csrfToken"] = wSession->sCSRFToken;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return HTTP_RET_200_OK;
}

eHTTP_RetCode WebClientHandler::processRPCRequest_AUTH(Request *request, string sSessionId)
{
    Authorization::Session::IAuth_Session *hSession = nullptr;
    uint64_t uMaxAge;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    eHTTP_RetCode eHTTPResponseCode = HTTP_RET_401_UNAUTHORIZED;

    // Authenticate...
    for (const uint32_t & passIdx : request->getAuthenticationsIdxs())
    {
        Authorization::DataStructs::AuthReason authReason;
        sSessionId = persistentAuthentication( getRequestVars(HTTP_VARS_POST)->getStringValue("user"),
                                               getRequestVars(HTTP_VARS_POST)->getStringValue("domain"),
                                               request->getAuthentication(passIdx),
                                               hSession, &authReason);

        (*(jPayloadOutStr->getValue()))["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
        (*(jPayloadOutStr->getValue()))["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);

        // Set the parameters once, the first time we see sessionid...
        if (!hSession && !sSessionId.empty())
        {
            WebSession * wSession = sessionsManager->openSession(sSessionId,&uMaxAge);
            if (wSession)
            {
                hSession = wSession->session;
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
                (*(jPayloadOutStr->getValue()))["csrfAuthConfirm"] = wSession->sCSRFAuthConfirmToken;
                eHTTPResponseCode = HTTP_RET_200_OK;
                sessionsManager->closeSession(sSessionId);
            }
        }
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

eHTTP_RetCode WebClientHandler::processRPCRequest_EXEC(Authorization::Session::IAuth_Session *hSession, Request *request)
{
    bool bDeleteTemporarySession = false;
    Memory::Streams::JSON_Streamable * jPayloadOutStr = new Memory::Streams::JSON_Streamable;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    eHTTP_RetCode eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
    std::string sMethodName = getRequestVars(HTTP_VARS_GET)->getStringValue("method");
    Json::Value jPayloadIn;
    Json::Reader reader;

    if (!getRequestVars(HTTP_VARS_POST)->getStringValue("payload").empty() && !reader.parse(getRequestVars(HTTP_VARS_POST)->getStringValue("payload"), jPayloadIn))
        return HTTP_RET_400_BAD_REQUEST;

    // Open a temporary session if there is no persistent session opened
    if (!hSession)
    {
        ////////////////////////////////////
        // Create Temporary Session
        ////////////////////////////////////
        hSession = new Authorization::Session::IAuth_Session;

        hSession->setAuthUser(getRequestVars(HTTP_VARS_POST)->getStringValue("user"));
        hSession->setAuthDomain(getRequestVars(HTTP_VARS_POST)->getStringValue("domain"));
        bDeleteTemporarySession = true;
    }

    std::set<uint32_t> extraTmpIndexes;
    for (const uint32_t & passIdx : request->getAuthenticationsIdxs())
    {
        Authorization::DataStructs::AuthReason authReason;

        if ((authReason=temporaryAuthentication(request->getAuthentication(passIdx),hSession)) == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
        {
            extraTmpIndexes.insert(passIdx);

            if (bDeleteTemporarySession)
            {
                // No persistent session, so make this AUTH persistent for this temporary session...
                hSession->registerPersistentAuthentication(passIdx,Authorization::DataStructs::AUTH_REASON_AUTHENTICATED);
            }
        }
        else
        {
            /*
             * // TODO:
            (*outJStreamable.getValue())["auth"][std::to_string(passIdx)]["reasonTxt"] = getAuthReasonText(authReason);
            (*outJStreamable.getValue())["auth"][std::to_string(passIdx)]["reasonVal"] = static_cast<Json::UInt>(authReason);
            */
        }
    }

    auto authorizer = authDomains->openDomain(hSession->getAuthDomain());
    if (authorizer)
    {
        Json::Value reasons;
        auto i = methodsManager->validateRPCMethodPerms( authorizer,  hSession, sMethodName, extraTmpIndexes, &reasons);
        authDomains->closeDomain(hSession->getAuthDomain());

        switch (i)
        {
        case VALIDATION_OK:
        {
            // TODO: retcode..
            hSession->updateLastActivity();
            methodsManager->runRPCMethod(authDomains,hSession->getAuthDomain(), hSession, sMethodName, jPayloadIn, jPayloadOutStr->getValue());
            eHTTPResponseCode = HTTP_RET_200_OK;
        }break;
        case VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
            eHTTPResponseCode = HTTP_RET_401_UNAUTHORIZED;
        }break;
        case VALIDATION_METHODNOTFOUND:
        default:
        {
            // not enough permissions.
            eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
        }break;
        }
    }
    else
    {
        eHTTPResponseCode = HTTP_RET_404_NOT_FOUND;
    }

    if (bDeleteTemporarySession)
    {
        ////////////////////////////////////
        // Temporary session mgmt
        ////////////////////////////////////
        delete hSession;
        hSession = nullptr;
    }
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

std::string WebClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Authentication &authData, Authorization::Session::IAuth_Session *session, Authorization::DataStructs::AuthReason * authReason)
{
    std::string sessionId;
    if (!session && authData.getPassIndex()!=0) return sessionId;

    *authReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    Json::Value payload;

    auto auth = authDomains->openDomain(domainName);
    if (auth)
    {
        *authReason = auth->authenticate(userName,authData.getUserPass(),authData.getPassIndex());
        authDomains->closeDomain(domainName);
    }

    if (!session && *authReason == Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
    {
        session = new Authorization::Session::IAuth_Session;
        session->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);
        sessionId = sessionsManager->addSession(session);
    }

    return sessionId;
}

Authorization::DataStructs::AuthReason WebClientHandler::temporaryAuthentication(const Authentication &authData, Authorization::Session::IAuth_Session *session)
{
    Authorization::DataStructs::AuthReason eReason;
    if (!session) return Authorization::DataStructs::AUTH_REASON_UNAUTHENTICATED;

    std::string userName = session->getAuthUser();
    std::string domainName = session->getAuthDomain();

    auto auth = authDomains->openDomain(domainName);
    if (!auth)
        eReason = Authorization::DataStructs::AUTH_REASON_INVALID_DOMAIN;
    else
    {
        eReason = auth->authenticate( userName,authData.getUserPass(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->closeDomain(domainName);
    }

    return eReason;
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

void WebClientHandler::setResourcesLocalPath(const std::string &value)
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
