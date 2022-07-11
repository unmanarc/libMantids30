#include "webclienthandler.h"
#include "mdz_xrpc_common/methodsmanager.h"

#include <boost/algorithm/string/predicate.hpp>
#include <mdz_mem_vars/b_mmap.h>
#include <mdz_xrpc_common/streamablejson.h>
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
using namespace Mantids::Protocols::HTTP;
using namespace Mantids::Memory;
using namespace Mantids::RPC::Web;
using namespace Mantids::RPC;
using namespace Mantids;
using namespace std;

WebClientHandler::WebClientHandler(void *parent, Memory::Streams::StreamableObject *sock) : HTTPv1_Server(sock)
{
    // TODO: rpc logs?
    bReleaseSessionHandler = false;
    bDestroySession = false;
    authSession = nullptr;
    webSession = nullptr;
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

Response::Status::eCode WebClientHandler::processClientRequest()
{
    Response::Status::eCode ret  = Response::Status::eCode::S_404_NOT_FOUND;
    bDestroySession = false;
    bReleaseSessionHandler = false;
    uint64_t uSessionMaxAge=0;

    // COOKIES:
    sSessionId = getRequestCookie("sessionId");
    // TODO: filter invalid session id formats.
    // HEADERS:
    sClientCSRFToken = getClientHeaderOption("CSRFToken");

    // POST VARS / EXTRA AUTHS:
    if (!extraCredentials.setAuthentications(getRequestVars(HTTP_VARS_POST)->getStringValue("extraAuth")))
        return Response::Status::eCode::S_400_BAD_REQUEST;

    // POST VARS / AUTH:
    if (!credentials.fromString(getRequestVars(HTTP_VARS_POST)->getStringValue("auth")))
        return Response::Status::eCode::S_400_BAD_REQUEST;

    // OPEN THE SESSION HERE:
    sessionOpen();

    std::string requestURI = getRequestURI();

    // Backward compatibility:
    // Detect if is /api, then process the JSON RPC Request.
    if (requestURI == "/api")
    {
        // Warn about this.
        log(LEVEL_WARN, "fileServer", 2048, "Calling deprecated /api: %s", requestURI.c_str());

        std::string mode = getRequestVars(HTTP_VARS_GET)->getStringValue("mode");
        if (mode == "EXEC")
            requestURI = "/japi_exec";
        else if (mode == "VERSION")
            requestURI = "/japi_version";
        else
            requestURI = "/japi_session";
    }

    // Detect if is /japi_exec, then process the JSON RPC Request.
    if (requestURI == "/japi_exec") ret = procJAPI_Exec(&extraCredentials,
                                                        getRequestVars(HTTP_VARS_GET)->getStringValue("method"),
                                                        getRequestVars(HTTP_VARS_POST)->getStringValue("payload")
                                                        );
    // Detect if is /japi_version, then process the JSON RPC Request.
    else if (requestURI == "/japi_version") ret = procJAPI_Version();
    // Detect if is /japi_session, then process the JSON RPC Request.
    else if (requestURI == "/japi_session") ret = procJAPI_Session();
    // Otherwise, process as web Resource
    else ret = procResource_File(&extraCredentials);

    /////////////////////////////////////////////////////////////////
    // CLEAN UPS...

    if (bDestroySession && webSession)
        log(LEVEL_INFO, "rpcServer", 2048, "Logged Out");

    // Release/Close the openned session.
    sessionRelease();
    sessionDestroy();

    return ret;
}

void WebClientHandler::sessionOpen()
{
    webSession = sessionsManager->openSession(sSessionId, &uSessionMaxAge);
    if (webSession)
    {
        bReleaseSessionHandler = true;

        // Copy the authenticated session:
        if (webSession->authSession)
            authSession = webSession->authSession;
    }
    else
    {
        if (sSessionId!="")
        {
            log(LEVEL_WARN, "rpcServer", 2048, "Requested session not found {sessionId=%s}",RPCLog::truncateSessionId(sSessionId).c_str());
            addCookieClearSecure("sessionId");
           // return Response::Status::eCode::S_404_NOT_FOUND;
        }
        sSessionId = ""; // INVALID SESSION ID.
    }
}

void WebClientHandler::sessionRelease()
{
    if (bReleaseSessionHandler)
    {
        // Set this cookie to report only to the javascript the remaining session time.
        Headers::Cookie simpleJSSecureCookie;
        simpleJSSecureCookie.setValue("1");
        simpleJSSecureCookie.setSecure(true);
        simpleJSSecureCookie.setHttpOnly(false);
        simpleJSSecureCookie.setExpirationInSeconds(uSessionMaxAge);
        simpleJSSecureCookie.setMaxAge(uSessionMaxAge);
        simpleJSSecureCookie.setSameSite( Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT);

        setResponseCookie("jsSessionTimeout",simpleJSSecureCookie);
        setResponseSecureCookie("sessionId", sSessionId, uSessionMaxAge);

        sessionsManager->releaseSession(sSessionId);
    }
}

void WebClientHandler::sessionDestroy()
{
    if (bDestroySession)
    {
        addCookieClearSecure("jsSessionTimeout");
        addCookieClearSecure("sessionId");
        log(LEVEL_DEBUG, "rpcServer", 2048, "Destroying session {sessionId=%s}",RPCLog::truncateSessionId(sSessionId).c_str());
        // TODO: redirect on logout?
        sessionsManager->destroySession(sSessionId);
    }
}

void replaceTagByJVar( std::string & content, const std::string & tag, const json & value, bool replaceFirst = false, const std::string & varName = "" )
{
    Json::FastWriter writer;
    std::string str = writer.write( value );
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    boost::replace_all(str,"<", "\\<");
    boost::replace_all(str,">", "\\>");

    if (!varName.empty() && varName.size()>1 && varName.at(0) == '/')
    {
        str = "<script>\nconst " + varName.substr(1) + " = " + str + ";\n</script>";
    }

    if (!replaceFirst)
        boost::replace_all(content,tag, str);
    else
        boost::replace_first(content,tag, str);
}



inline unsigned char get16Value(unsigned char byte)
{
    if (byte>='A' && byte<='F') return byte-'A'+10;
    else if (byte>='a' && byte<='f') return byte-'a'+10;
    else if (byte>='0' && byte<='9') return byte-'0';
    return 0;
}

inline unsigned char hex2uchar(const std::string &t1, const std::string &t2)
{
    return get16Value(t1.c_str()[0])*0x10+get16Value(t2.c_str()[0]);
}

void replaceHexCodes( std::string &content )
{
    boost::match_results<string::const_iterator> whatStaticText;
    boost::regex exStaticJsonFunction("\\\\0*x(?<V1>[0123456789ABCDEF])(?<V2>[0123456789ABCDEF])",boost::regex::icase);
    boost::match_flag_type flags = boost::match_default;

    for (string::const_iterator start = content.begin(), end =  content.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonFunction, flags); // FIND REGEXP
         start = content.begin(), end =  content.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string v1  = string(whatStaticText[1].first, whatStaticText[1].second);
        string v2  = string(whatStaticText[2].first, whatStaticText[2].second);

        std::string replSrc(1,hex2uchar(v1,v2));
        boost::replace_all(content,fulltag, replSrc);
    }

}

// TODO: documentar los privilegios cargados de un usuario
Response::Status::eCode WebClientHandler::procResource_HTMLIEngine( const std::string & sRealFullPath, MultiAuths *extraAuths)
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
            log(LEVEL_ERR,"fileServer", 2048, "file not found: %s",sRealFullPath.c_str());
            return Response::Status::eCode::S_404_NOT_FOUND;
        }
        // Pass the file to a string.
        fileContent = std::string((std::istreambuf_iterator<char>(fileStream)),std::istreambuf_iterator<char>());
        fileStream.close();
    }

    // PRECOMPILE _STATIC_TEXT
    boost::match_flag_type flags = boost::match_default;

    // CINC PROCESSOR:
    //boost::regex exStaticText("<CINC_(?<TAGOPEN>[^>]*)>(?<INCPATH>[^<]+)<\\/CINC_(?<TAGCLOSE>[^>]*)>",boost::regex::icase);

    boost::regex exStaticText("<\\%?include(?<SCRIPT_TAG_NAME>[^\\:]*):[ ]*(?<PATH>[^\\%]+)[ ]*\\%>",boost::regex::icase);

    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticText, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string tag          = string(whatStaticText[1].first, whatStaticText[1].second);
        string includePath  = string(whatStaticText[2].first, whatStaticText[2].second);
//      string tagClose     = string(whatStaticText[3].first, whatStaticText[3].second);

        // GET THE TAG DATA HERE...
        // The path is relative to resourcesLocalPath (beware: admits transversal)
        std::ifstream fileIncludeStream(resourcesLocalPath + includePath);

        if (fileIncludeStream.is_open())
        {
            std::string includeFileContent((std::istreambuf_iterator<char>(fileIncludeStream)),std::istreambuf_iterator<char>());
            if (!tag.empty() && tag.size()>1 && tag.at(0) == '/')
                boost::replace_all(fileContent,fulltag, "<" + tag.substr(1) + ">" + includeFileContent + "</" + tag.substr(1) + ">" );
            else
                boost::replace_all(fileContent,fulltag, includeFileContent);
        }
        else
        {
            boost::replace_all(fileContent,fulltag, "<!-- HTMLI ENGINE ERROR (FILE NOT FOUND): " + includePath + " -->");

            log(LEVEL_ERR,"fileserver", 2048, "file not found: %s",sRealFullPath.c_str());
        }
    }

    json jVars,jNull;
    jVars["softwareVersion"]   = softwareVersion;
    jVars["csrfToken"]         = webSession?webSession->sCSRFToken:jNull;
    jVars["user"]              = authSession?authSession->getUserDomainPair().first:jNull;
    jVars["domain"]            = authSession?authSession->getUserDomainPair().second:jNull;
    jVars["maxAge"]            = (Json::UInt64)(webSession?uSessionMaxAge:0);
    jVars["userAgent"]         = getRequestActiveObjects().USER_AGENT;
    jVars["userIP"]            = userIP;
    jVars["userTLSCommonName"] = userTLSCommonName;

    // %JVAR PROCESSOR:
    boost::regex exStaticJsonInputVar("<\\%?jvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonInputVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Report as not found.
        if ( !jVars.isMember(varName) )
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,jVars[varName],false,scriptVarName);
        }
    }

    // %JSESSVAR PROCESSOR:
    boost::regex exStaticJsonSessionVar("<\\%?jsessvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonSessionVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Report as not found.
        if ( ! (authSession && authSession->getSessionVarExist(varName))  )
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,authSession->getSessionVarValue(varName),false,scriptVarName);
        }
    }

    // %JPOSTVAR PROCESSOR:
    boost::regex exStaticJsonPostVar("<\\%?jpostvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonPostVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Obtain using POST Vars...
        if (getRequestVars(HTTP_VARS_POST)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,getRequestVars(HTTP_VARS_POST)->getStringValue(varName));
        }
        // Report as not found.
        else
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Post variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
    }

    // %JGETVAR PROCESSOR:
    boost::regex exStaticJsonGetVar("<\\%?jgetvar(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<VAR_NAME>[^\\%]+)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonGetVar, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag      = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string varName       = string(whatStaticText[2].first, whatStaticText[2].second);

        // Obtain using POST Vars...
        if (getRequestVars(HTTP_VARS_GET)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,getRequestVars(HTTP_VARS_GET)->getStringValue(varName));
        }
        // Report as not found.
        else
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Get variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
    }

    //%JFUNC PROCESSOR:
    // TODO: como revisar que realmente termine en ) y no haya un ) dentro del json
    boost::regex exStaticJsonFunction("<\\%jfunc(?<SCRIPT_VAR_NAME>[^\\:]*):[ ]*(?<FUNCTION_NAME>[^\\(]+)\\((?<FUNCTION_VALUE>[^\\)]*)\\)[ ]*\\%>",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticJsonFunction, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        string scriptVarName = string(whatStaticText[1].first, whatStaticText[1].second);
        string functionName  = string(whatStaticText[2].first, whatStaticText[2].second);
        string functionInput = string(whatStaticText[3].first, whatStaticText[3].second);

        replaceHexCodes(functionInput);

        Memory::Streams::StreamableJSON jPayloadOutStr;
        procJAPI_Exec(extraAuths,functionName,functionInput, &jPayloadOutStr);
        replaceTagByJVar(fileContent,fulltag,*(jPayloadOutStr.getValue()),true,scriptVarName);
    }


    // Sanitize <script></script>
  /*  boost::regex exStaticScriptSanitizer("</script>[\\ \n\r\t]*<script>[\n]?",boost::regex::icase);
    for (string::const_iterator start = fileContent.begin(), end =  fileContent.end(); //
         boost::regex_search(start, end, whatStaticText, exStaticScriptSanitizer, flags); // FIND REGEXP
         start = fileContent.begin(), end =  fileContent.end()) // RESET AND RECHECK EVERYTHING
    {
        string fulltag       = string(whatStaticText[0].first, whatStaticText[0].second);
        boost::ireplace_all(fileContent,fulltag, "");
    }*/




    // Update last activity on each page load.
    if (authSession)
        authSession->updateLastActivity();

    // Stream the generated content...
    getResponseDataStreamer()->writeString(fileContent);
    return Response::Status::eCode::S_200_OK;
}

Response::Status::eCode WebClientHandler::procJAPI_Session()
{
    std::string sMode;
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;

    // GET VARS:
    sMode = getRequestVars(HTTP_VARS_GET)->getStringValue("mode");

    // In AUTHCSRF we take the session ID from post variable (not cookie).
    if (sSessionId.empty() && usingCSRFToken && sMode == "AUTHCSRF")
    {
        sSessionId = getRequestVars(HTTP_VARS_POST)->getStringValue("sessionId");
        // Open the session again (with the post value):
        sessionOpen();
    }

    // TODO: active check of origin to avoid cross-domain

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
            if (!webSession->bAuthTokenConfirmed && sMode == "AUTHCSRF")
            {
                if (webSession->confirmAuthCSRFToken(sClientCSRFToken))
                {
                    // Now this method will fixate the introduced session in the browser...
                    log(LEVEL_DEBUG, "rpcServer", 2048, "CSRF Confirmation Token OK");
                    setResponseSecureCookie("sessionId", sSessionId, uSessionMaxAge);
                    eHTTPResponseCode = Response::Status::eCode::S_200_OK;
                }
                else
                {
                    log(LEVEL_ERR,  "rpcServer", 2048, "Invalid CSRF Confirmation Token {mode=%s}", sMode.c_str());
                    eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
                }
            }
            // Session found and auth token already confirmed, CSRF token must match session.
            else if (webSession->bAuthTokenConfirmed && webSession->validateCSRFToken(sClientCSRFToken))
            {
                log(LEVEL_DEBUG,  "rpcServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (webSession->bAuthTokenConfirmed && !webSession->validateCSRFToken(sClientCSRFToken) && !(sMode == "CSRFTOKEN"))
            {
                log(LEVEL_ERR,  "rpcServer", 2048, "Invalid CSRF Token {mode=%s}", sMode.c_str());
            }
            // We are just going to obtain the CSRF Token
            else if (webSession->bAuthTokenConfirmed && (sMode == "CSRFTOKEN"))
                csrfValidationOK = true;
        }
        else if ( sMode == "LOGIN" )
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
        if ( !webSession && sMode == "LOGIN" && (!usingCSRFToken || sClientCSRFToken == "00112233445566778899"))
            eHTTPResponseCode = procJAPI_Session_LOGIN(credentials);
        /////////////////////////////////////////////////////////////////
        // POST PERSISTENT AUTHENTICATION...
        else if (webSession && webSession->bAuthTokenConfirmed && sMode == "POSTLOGIN")
            eHTTPResponseCode = procJAPI_Session_POSTLOGIN(credentials);
        /////////////////////////////////////////////////////////////////
        // CHANGE MY ACCOUNT PASSWORD...
        else if (webSession && authSession && // exist a websession+authSession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 authSession->getIsFullyLoggedIn(Mantids::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "CHPASSWD") // CHPASSWD command
            eHTTPResponseCode = procJAPI_Session_CHPASSWD(credentials);
        /////////////////////////////////////////////////////////////////
        // TEST MY ACCOUNT PASSWORD...
        else if (webSession && authSession && // exist a websession+authSession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 authSession->getIsFullyLoggedIn(Mantids::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "TESTPASSWD") // TESTPASSWD command
            eHTTPResponseCode = procJAPI_Session_TESTPASSWD(credentials);
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        // GET MY ACCOUNT PASSWORD LIST/DESCRIPTION...
        else if (webSession && authSession && // exist a websession+authSession
                 webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 authSession->getIsFullyLoggedIn(Mantids::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "PASSWDLIST") // PASSWDLIST command
            eHTTPResponseCode = procJAPI_Session_PASSWDLIST();
        /////////////////////////////////////////////////////////////////
        // CSRF TOKEN REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( webSession && webSession->bAuthTokenConfirmed && sMode == "CSRFTOKEN" )
            eHTTPResponseCode = procJAPI_Session_CSRFTOKEN();
        /////////////////////////////////////////////////////////////////
        // AUTH INFO REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( webSession && webSession->bAuthTokenConfirmed && sMode == "AUTHINFO" )
            eHTTPResponseCode = procJAPI_Session_AUTHINFO();
        /////////////////////////////////////////////////////////////////
        // PERSISTENT SESSION LOGOUT
        else if ( webSession && sMode == "LOGOUT" )
        {
            eHTTPResponseCode = Response::Status::eCode::S_200_OK;
            bDestroySession = true;
        }
    }

    // return the HTTP response code.
    return eHTTPResponseCode;
}

bool WebClientHandler::csrfValidate()
{
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
            // Session found and auth token already confirmed, CSRF token must match session.
            if (webSession->bAuthTokenConfirmed && webSession->validateCSRFToken(sClientCSRFToken))
            {
                log(LEVEL_DEBUG, "rpcServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (webSession->bAuthTokenConfirmed && !webSession->validateCSRFToken(sClientCSRFToken))
            {
                log(LEVEL_ERR, "rpcServer", 2048, "Invalid CSRF Token {mode=EXEC}");
            }
        }
    }

    return csrfValidationOK;
}

Response::Status::eCode WebClientHandler::procResource_File(MultiAuths *extraAuths)
{
    // WEB RESOURCE MODE:
    Response::Status::eCode ret  = Response::Status::eCode::S_404_NOT_FOUND;
    sLocalRequestedFileInfo fileInfo;
    uint64_t uMaxAge=0;

    // if there are no web resources path, return 404 without data.
    if (resourcesLocalPath.empty())
            return Response::Status::eCode::S_404_NOT_FOUND;

    if ( //staticContent ||
         (getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, ".html") ||
          getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, "index.html") ||
          getLocalFilePathFromURI2(resourcesLocalPath, &fileInfo, "")
          ) && !fileInfo.isDir
         )
    {
        // Evaluate...
        ResourcesFilter::sFilterEvaluation e;

        // If there is any session (hSession), then get the authorizer from the session
        // if not, the authorizer is null.
        Mantids::Authentication::Manager * authorizer = authSession?authDomains->openDomain(authSession->getAuthDomain()) : nullptr;

        // if there is any resource filter, evaluate the sRealRelativePath with the action to be taken for that file
        // it will proccess this according to the authorization session
        if ( resourceFilter )
            e = resourceFilter->evaluateAction(fileInfo.sRealRelativePath, authSession, authorizer);

        // If the element is accepted (during the filter)
        if (e.accept)
        {
            // and there is not redirect's, the resoponse code will be 200 (OK)
            if (e.redirectLocation.empty())
                ret = Response::Status::eCode::S_200_OK;
            else // otherwise you will need to redirect.
                ret = setResponseRedirect( e.redirectLocation );
        }
        else // If not, drop a 403 (forbidden)
            ret = Response::Status::eCode::S_403_FORBIDDEN;

        log(LEVEL_DEBUG,"fileServer", 2048, "R/ - LOCAL - %03d: %s",Response::Status::getHTTPStatusCodeTranslation(ret),fileInfo.sRealFullPath.c_str());
    }
    else
    {
        // File not found at this point (404)
        log(LEVEL_WARN,"fileServer", 65535, "R/404: %s",getRequestURI().c_str());
    }

    if (ret != Response::Status::eCode::S_200_OK)
    {
        // For NON-200 responses, will stream nothing....
        setResponseDataStreamer(nullptr,false);
    }

    // If the URL is going to process the Interactive HTML Engine,
    // and the document content is text/html, then, process it as HTMLIEngine:
    if ( useHTMLIEngine &&
         getContentType() == "text/html" ) // The content type has changed during the map.
        procResource_HTMLIEngine(fileInfo.sRealFullPath,extraAuths);

    // And if the file is not found and there are redirections, set the redirection:
    if (ret==Response::Status::eCode::S_404_NOT_FOUND && !redirectOn404.empty())
    {
        ret = setResponseRedirect( redirectOn404 );
    }

    // Log the response.
    log(ret==Response::Status::eCode::S_200_OK?LEVEL_INFO:LEVEL_WARN,
        "fileServer", 2048, "R/%03d: %s",
        Response::Status::getHTTPStatusCodeTranslation(ret),
        ret==Response::Status::eCode::S_200_OK?fileInfo.sRealRelativePath.c_str():getRequestURI().c_str());

    return ret;
}

Response::Status::eCode WebClientHandler::procJAPI_Version()
{
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["version"]  = softwareVersion;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return Response::Status::eCode::S_200_OK;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_AUTHINFO()
{
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    (*(jPayloadOutStr->getValue()))["user"]   = !authSession?"":authSession->getUserDomainPair().first;
    (*(jPayloadOutStr->getValue()))["domain"] = !authSession?"":authSession->getUserDomainPair().second;
    (*(jPayloadOutStr->getValue()))["maxAge"] = (Json::UInt64)uSessionMaxAge;

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return Response::Status::eCode::S_200_OK;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_CSRFTOKEN()
{
    // On Page Load...
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["csrfToken"] = webSession->sCSRFToken;
    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);

    // Update last activity on each page load.
    if (authSession)
        authSession->updateLastActivity();

    return Response::Status::eCode::S_200_OK;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_LOGIN(const Authentication & auth)
{
    Mantids::Authentication::Reason authReason;
    uint64_t uMaxAge;
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;


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

                log(LEVEL_INFO,  "rpcServer", 2048, "Logged in, waiting for the next authentication factor {val=%d,txt=%s}",
                    JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }
            else
            {
                log(LEVEL_INFO,  "rpcServer", 2048, "Logged in {val=%d,txt=%s}", JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }

            eHTTPResponseCode = Response::Status::eCode::S_200_OK;

            sessionsManager->releaseSession(sSessionId);
        }
    }
    else
    {
        // TODO: for better log, remove usage of , in user/domain
        log(LEVEL_WARN,  "rpcServer", 2048, "Invalid Login Attempt {val=%d,txt=%s,user=%s,domain=%s}",
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0),
            JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""),
            user.c_str(),
            domain.c_str());

    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_POSTLOGIN(const Authentication &auth)
{
    Mantids::Authentication::Reason authReason;
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;

    // Authenticate...
    // We fill the sSessionId in case we want to destroy it with bDestroySession
    sSessionId = persistentAuthentication( authSession->getAuthUser(),
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

            log(LEVEL_INFO,  "rpcServer", 2048, "Authentication factor (%d) OK, waiting for the next authentication factor {val=%d,txt=%s}", auth.getPassIndex(), i.first, i.second.c_str());
        }
        else
        {
            log(LEVEL_INFO,  "rpcServer", 2048, "Authentication factor (%d) OK, Logged in.", auth.getPassIndex());
        }
        eHTTPResponseCode = Response::Status::eCode::S_200_OK;
    }
    else
    {
        log(LEVEL_WARN,  "rpcServer", 2048, "Authentication error on factor #(%d), Logged out {val=%d,txt=%s}",auth.getPassIndex(),
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt","")
            );

        // Mark to Destroy the session if the chpasswd is invalid...
        bDestroySession = true;
        eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::Status::eCode WebClientHandler::procJAPI_Exec(MultiAuths *extraAuths,
                                                     std::string sMethodName,
                                                     std::string sPayloadIn,
                                                     Memory::Streams::StreamableJSON * jPayloadOutStr
                                                     )
{
    bool useExternalPayload = jPayloadOutStr?true:false;

    // External payloads does not csrf validate. (eg. inline html execution)
    if (!useExternalPayload)
    {
        if (!csrfValidate())
            return Response::Status::eCode::S_404_NOT_FOUND;
    }

    if (!useExternalPayload) jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;

    json jPayloadIn;
    Mantids::Helpers::JSONReader2 reader;

    std::string  userName   = getRequestVars(HTTP_VARS_POST)->getStringValue("user");
    std::string domainName  = getRequestVars(HTTP_VARS_POST)->getStringValue("domain");

    // If there is a session, overwrite the user/domain inputs...
    if (authSession)
    {
        userName = authSession->getAuthUser();
        domainName = authSession->getAuthDomain();
    }

    if (!getRequestVars(HTTP_VARS_POST)->getStringValue("payload").empty() && !reader.parse(sPayloadIn, jPayloadIn))
    {
        log(LEVEL_ERR,  "rpcServer", 2048, "Invalid JSON Payload for execution {method=%s}", sMethodName.c_str());
        return Response::Status::eCode::S_400_BAD_REQUEST;
    }

    if (!authDomains)
    {
        log(LEVEL_CRITICAL,  "rpcServer", 2048, "No authentication domain list exist.");
        return Response::Status::eCode::S_500_INTERNAL_SERVER_ERROR;
    }

    if (methodsManager->getMethodRequireFullAuth(sMethodName) && !authSession)
    {
        log(LEVEL_ERR, "rpcServer", 2048, "This method requires full authentication {method=%s}", sMethodName.c_str());
        // Method not available for this null session..
        return Response::Status::eCode::S_404_NOT_FOUND;
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
            log(LEVEL_INFO, "rpcServer", 2048, "Adding valid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids::Authentication::getReasonText(authReason));
            extraTmpIndexes.insert(passIdx);
        }
        else
        {
            log(LEVEL_WARN, "rpcServer", 2048, "Rejecting invalid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids::Authentication::getReasonText(authReason));
        }
    }

    auto authorizer = authDomains->openDomain(domainName);
    if (authorizer)
    {
        json reasons;

        // Validate that the RPC method is ready to go (fully authorized and no password is expired).
        auto i = methodsManager->validateRPCMethodPerms( authorizer,  authSession, sMethodName, extraTmpIndexes, &reasons );

        authDomains->releaseDomain(domainName);

        switch (i)
        {
        case MethodsManager::VALIDATION_OK:
        {
            if (authSession)
                authSession->updateLastActivity();

            log(LEVEL_INFO, "rpcServer", 2048, "Executing Web Method {method=%s}", sMethodName.c_str());
            log(LEVEL_DEBUG, "rpcServer", 8192, "Executing Web Method - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(jPayloadIn).c_str());

            auto start = chrono::high_resolution_clock::now();
            auto finish = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = finish - start;

            switch (methodsManager->runRPCMethod(authDomains,domainName, authSession, sMethodName, jPayloadIn, jPayloadOutStr->getValue()))
            {
            case Mantids::RPC::MethodsManager::METHOD_RET_CODE_SUCCESS:

                finish = chrono::high_resolution_clock::now();
                elapsed = finish - start;

                log(LEVEL_INFO, "rpcServer", 2048, "Web Method executed OK {method=%s, elapsedMS=%f}", sMethodName.c_str(),elapsed.count());
                log(LEVEL_DEBUG, "rpcServer", 8192, "Web Method executed OK - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(jPayloadOutStr->getValue()).c_str());

                eHTTPResponseCode = Response::Status::eCode::S_200_OK;
                break;
            case Mantids::RPC::MethodsManager::METHOD_RET_CODE_METHODNOTFOUND:
                log(LEVEL_ERR, "rpcServer", 2048, "Web Method not found {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;
                break;
            case Mantids::RPC::MethodsManager::METHOD_RET_CODE_INVALIDDOMAIN:
                // This code should never be executed... <
                log(LEVEL_ERR, "rpcServer", 2048, "Domain not found during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;
                break;
            default:
                log(LEVEL_ERR, "rpcServer", 2048, "Unknown error during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
                break;
            }
        }break;
        case MethodsManager::VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
            log(LEVEL_ERR, "rpcServer", 8192, "Not authorized to execute method {method=%s,reasons=%s}", sMethodName.c_str(),Mantids::Helpers::jsonToString(reasons).c_str());
            eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
        }break;
        case MethodsManager::VALIDATION_METHODNOTFOUND:
        default:
        {
            log(LEVEL_ERR, "rpcServer", 2048, "Method not found {method=%s}", sMethodName.c_str());
            // not enough permissions.
            eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;
        }break;
        }
    }
    else
    {
        log(LEVEL_ERR, "rpcServer", 2048, "Domain not found {method=%s}", sMethodName.c_str());

        // Domain Not found.
        eHTTPResponseCode = Response::Status::eCode::S_404_NOT_FOUND;
    }

    if (!useExternalPayload)
    {
        setResponseDataStreamer(jPayloadOutStr,true);
        setResponseContentType("application/json",true);
    }
    return eHTTPResponseCode;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_CHPASSWD(const Authentication &oldAuth)
{
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;

    if (!authSession)
        return eHTTPResponseCode;

    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    Authentication newAuth;

    // POST VARS / AUTH:
    if (!newAuth.fromString(getRequestVars(HTTP_VARS_POST)->getStringValue("newAuth")))
    {
        log(LEVEL_ERR, "rpcServer", 2048, "Invalid JSON Parsing for new credentials item");
        return Response::Status::eCode::S_400_BAD_REQUEST;
    }

    if (oldAuth.getPassIndex()!=newAuth.getPassIndex())
    {
        log(LEVEL_ERR, "rpcServer", 2048, "Provided credential index differs from new credential index.");
        return Response::Status::eCode::S_400_BAD_REQUEST;
    }

    uint32_t credIdx = newAuth.getPassIndex();

    auto domainAuthenticator = authDomains->openDomain(authSession->getAuthDomain());
    if (domainAuthenticator)
    {
        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = userIP;
        clientDetails.sTLSCommonName = userTLSCommonName;
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
                log(LEVEL_INFO, "rpcServer", 2048, "Password change requested {idx=%d,result=1}",credIdx);
            else
                log(LEVEL_ERR, "rpcServer", 2048, "Password change failed due to internal error {idx=%d,result=0}",credIdx);

            eHTTPResponseCode = Response::Status::eCode::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, "rpcServer", 2048, "Password change failed, bad incomming credentials {idx=%d,reason=%s}",credIdx,Mantids::Authentication::getReasonText(authReason));

            // Mark to Destroy the session if the chpasswd is invalid...
            bDestroySession = true;
            eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
        }


        authDomains->releaseDomain(authSession->getAuthDomain());
    }
    else
    {
        log(LEVEL_ERR, "rpcServer", 2048, "Password change failed, domain authenticator not found {idx=%d}",credIdx);
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;

}

Response::Status::eCode WebClientHandler::procJAPI_Session_TESTPASSWD(const Authentication &auth)
{
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;

    if (!authSession) return eHTTPResponseCode;

    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
    jPayloadOutStr->setFormatted(useFormattedJSONOutput);

    auto domainAuthenticator = authDomains->openDomain(authSession->getAuthDomain());
    if (domainAuthenticator)
    {
        Mantids::Authentication::sClientDetails clientDetails;
        clientDetails.sIPAddr = userIP;
        clientDetails.sTLSCommonName = userTLSCommonName;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        auto authReason = domainAuthenticator->authenticate(appName,clientDetails,authSession->getAuthUser(),auth.getPassword(),0);
        if (IS_PASSWORD_AUTHENTICATED(authReason))
        {
            log(LEVEL_INFO, "rpcServer", 2048, "Password validation requested {idx=%d,result=1}",auth.getPassIndex());
            //(*(jPayloadOutStr->getValue()))["ok"] = true;
            eHTTPResponseCode = Response::Status::eCode::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, "rpcServer", 2048, "Password validation failed, bad incomming credentials {idx=%d,reason=%s}",auth.getPassIndex(),Mantids::Authentication::getReasonText(authReason));

            // Mark to destroy the session if the test password is invalid...
            bDestroySession = true;
            eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;
        }
        (*(jPayloadOutStr->getValue()))["ok"] = true;


        authDomains->releaseDomain(authSession->getAuthDomain());
    }
    else
    {
        log(LEVEL_ERR, "rpcServer", 2048, "Password validation failed, domain authenticator not found {idx=%d}",auth.getPassIndex());
    }

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

Response::Status::eCode WebClientHandler::procJAPI_Session_PASSWDLIST()
{
    Response::Status::eCode eHTTPResponseCode = Response::Status::eCode::S_401_UNAUTHORIZED;

    if (!authSession) return eHTTPResponseCode;

    eHTTPResponseCode = Response::Status::eCode::S_200_OK;
    Memory::Streams::StreamableJSON * jPayloadOutStr = new Memory::Streams::StreamableJSON;
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
        eHTTPResponseCode = Response::Status::eCode::S_500_INTERNAL_SERVER_ERROR;

    setResponseDataStreamer(jPayloadOutStr,true);
    setResponseContentType("application/json",true);
    return eHTTPResponseCode;
}

void WebClientHandler::setSessionsManagger(SessionsManager *value)
{
    sessionsManager = value;
}

void WebClientHandler::setUserIP(const std::string &value)
{
    userIP = value;
}

std::string WebClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Authentication &authData, Mantids::Authentication::Session *lAuthSession, Mantids::Authentication::Reason * authReason)
{
    json payload;
    std::string sessionId;
    std::map<uint32_t,std::string> stAccountPassIndexesUsedForLogin;

    // Don't allow other than 0 idx in the first auth. (Return empty session ID with internal error.)
    if (!lAuthSession && authData.getPassIndex()!=0)
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
        clientDetails.sIPAddr = userIP;
        clientDetails.sTLSCommonName = userTLSCommonName;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        *authReason = domainAuthenticator->authenticate(appName,clientDetails,userName,authData.getPassword(),authData.getPassIndex(), Mantids::Authentication::MODE_PLAIN,"",&stAccountPassIndexesUsedForLogin);

        authDomains->releaseDomain(domainName);
    }

    if ( Mantids::Authentication::IS_PASSWORD_AUTHENTICATED( *authReason ) )
    {
        // If not exist an authenticated session, create a new one.
        if (!lAuthSession)
        {
            lAuthSession = new Mantids::Authentication::Session(appName);
            lAuthSession->setIsPersistentSession(true);
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);

            // The first pass/time the list of idx should be filled into.
            if (authData.getPassIndex()==0)
                lAuthSession->setRequiredLoginIdx(stAccountPassIndexesUsedForLogin);

            // Add to session manager (creates web session).
            sessionId = sessionsManager->createWebSession(lAuthSession);

            // Destroy the authentication session if the web session was not inserted.
            if (sessionId == "")
            {
                delete lAuthSession;
                lAuthSession = nullptr;
            }
        }
        else
        {
            // If exist, just register the current authentication into that session and return the current sessionid
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.getPassIndex(),*authReason);
            sessionId = lAuthSession->getSessionId();
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
        clientDetails.sIPAddr = userIP;
        clientDetails.sTLSCommonName = userTLSCommonName;
        clientDetails.sUserAgent = getRequestActiveObjects().USER_AGENT;

        eReason = auth->authenticate( appName, clientDetails, userName,authData.getPassword(),authData.getPassIndex()); // Authenticate in a non-persistent fashion.
        authDomains->releaseDomain(domainName);
    }

    return eReason;
}

void WebClientHandler::log(eLogLevels logSeverity,  const std::string & module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);

    if (rpcLog) rpcLog->logVA( logSeverity,
                               userIP,
                               !authSession?"" : authSession->getSessionId(),
                               !authSession?"" : authSession->getAuthUser(),
                               !authSession?"" : authSession->getAuthDomain(),
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
    userTLSCommonName = value;
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
    if (value.empty())
    {
        resourcesLocalPath = "";
        return;
    }

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



