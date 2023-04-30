#include "clienthandler.h"
#include <Mantids29/Auth/data.h>
#include <Mantids29/Protocol_HTTP/rsp_status.h>
#include <Mantids29/API_Monolith/methodshandler.h>

#include <Mantids29/Memory/b_mmap.h>
#include <Mantids29/Memory/streamablejson.h>
#include <Mantids29/Helpers/crypto.h>
#include <Mantids29/Helpers/json.h>

#include <memory>
#include <stdarg.h>
#include <fstream>
#include <streambuf>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <utility>

#ifdef _WIN32
#include <stdlib.h>
// TODO: check if _fullpath mitigate transversal.
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

using namespace Mantids29::Program::Logs;
using namespace Mantids29::Network;
using namespace Mantids29::Network::Protocols;
using namespace Mantids29::Network::Protocols::HTTP;
using namespace Mantids29::Memory;
using namespace Mantids29::Network::Servers::WebMonolith;
using namespace Mantids29;
using namespace std;

ClientHandler::ClientHandler(void *parent, Memory::Streams::StreamableObject *sock) : HTTPv1_Server(sock)
{
    // TODO: rpc logs?
    m_resourceFilter = nullptr;
}

ClientHandler::~ClientHandler()
{
}

void ClientHandler::setAuthenticators(Mantids29::Authentication::Domains *authenticator)
{
    m_authDomains = authenticator;
}

Status::eRetCode ClientHandler::procHTTPClientContent()
{
    HTTP::Status::eRetCode ret  = HTTP::Status::S_404_NOT_FOUND;
    m_destroySession = false;
    m_releaseSessionHandler = false;
    uint64_t uSessionMaxAge=0;

    // COOKIES:
    m_sessionId = m_clientRequest.getCookie("sessionId");
    // TODO: filter invalid session id formats.
    // HEADERS:
    m_clientCSRFToken = m_clientRequest.getHeaderOption("CSRFToken");

    // POST VARS / EXTRA AUTHS:
    if (!m_extraCredentials.setAuthentications(m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("extraAuth")))
        return HTTP::Status::S_400_BAD_REQUEST;

    // POST VARS / AUTH:
    if (!m_credentials.setJsonString(m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("auth")))
        return HTTP::Status::S_400_BAD_REQUEST;

    // OPEN THE SESSION HERE:
    sessionOpen();

    std::string requestURI = m_clientRequest.getURI();

    // Backward compatibility:
    // Detect if is /api, then process the JSON RPC Request.
    if (requestURI == "/api")
    {
        // Warn about this.
        log(LEVEL_WARN, "fileServer", 2048, "Calling deprecated /api: %s", requestURI.c_str());

        std::string mode = m_clientRequest.getVars(HTTP_VARS_GET)->getStringValue("mode");
        if (mode == "EXEC")
            requestURI = "/japi_exec";
        else if (mode == "VERSION")
            requestURI = "/japi_version";
        else
            requestURI = "/japi_session";
    }

    // Detect if is /japi_exec, then process the JSON RPC Request.
    if (requestURI == "/japi_exec") ret = procJAPI_Exec(&m_extraCredentials,
                                                        m_clientRequest.getVars(HTTP_VARS_GET)->getStringValue("method"),
                                                        m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("payload")
                                                        );
    // Detect if is /japi_version, then process the JSON RPC Request.
    else if (requestURI == "/japi_version") ret = procJAPI_Version();
    // Detect if is /japi_session, then process the JSON RPC Request.
    else if (requestURI == "/japi_session") ret = procJAPI_Session();
    // Otherwise, process as web Resource
    else ret = procResource_File(&m_extraCredentials);

    /////////////////////////////////////////////////////////////////
    // CLEAN UPS...
    if (m_destroySession && m_webSession)
        log(LEVEL_INFO, "apiServer", 2048, "Logged Out");

    // Release/Close the openned session.
    sessionRelease();
    sessionDestroy();

    return ret;
}

void ClientHandler::replaceHexCodes(std::string &content)
{
    auto hex2uchar = [](const std::string &t1, const std::string &t2) -> unsigned char {
        auto get16Value = [](unsigned char byte) -> unsigned char {
            if (byte >= 'A' && byte <= 'F') return byte - 'A' + 10;
            else if (byte >= 'a' && byte <= 'f') return byte - 'a' + 10;
            else if (byte >= '0' && byte <= '9') return byte - '0';
            return 0;
        };

        return get16Value(t1.c_str()[0]) * 0x10 + get16Value(t2.c_str()[0]);
    };

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

void ClientHandler::sessionOpen()
{
    m_webSession = m_sessionsManager->openSession(m_sessionId, &m_sessionMaxAge);
    if (m_webSession)
    {
        m_releaseSessionHandler = true;

        // Copy the authenticated session:
        if (m_webSession->authSession)
            m_authSession = m_webSession->authSession;
    }
    else
    {
        if (m_sessionId!="")
        {
            log(LEVEL_WARN, "apiServer", 2048, "Requested session not found {sessionId=%s}",RPCLog::truncateSessionId(m_sessionId).c_str());
            m_serverResponse.addCookieClearSecure("sessionId");
            // return HTTP::Status::S_404_NOT_FOUND;
        }
        m_sessionId = ""; // INVALID SESSION ID.
    }
}

void ClientHandler::sessionRelease()
{
    if (m_releaseSessionHandler)
    {
        // Set this cookie to report only to the javascript the remaining session time.
        Headers::Cookie simpleJSSecureCookie;
        simpleJSSecureCookie.setValue("1");
        simpleJSSecureCookie.setSecure(true);
        simpleJSSecureCookie.setHttpOnly(false);
        simpleJSSecureCookie.setExpirationFromNow(m_sessionMaxAge);
        simpleJSSecureCookie.setMaxAge(m_sessionMaxAge);
        simpleJSSecureCookie.setSameSite( Protocols::HTTP::Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT);

        m_serverResponse.setCookie("jsSessionTimeout",simpleJSSecureCookie);
        m_serverResponse.setSecureCookie("sessionId", m_sessionId, m_sessionMaxAge);

        m_sessionsManager->releaseSession(m_sessionId);
    }
}

void ClientHandler::sessionDestroy()
{
    if (m_destroySession)
    {
        m_serverResponse.addCookieClearSecure("jsSessionTimeout");
        m_serverResponse.addCookieClearSecure("sessionId");
        log(LEVEL_DEBUG, "apiServer", 2048, "Destroying session {sessionId=%s}",RPCLog::truncateSessionId(m_sessionId).c_str());
        // TODO: redirect on logout?
        m_sessionsManager->destroySession(m_sessionId);
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



// TODO: documentar los privilegios cargados de un usuario
Status::eRetCode ClientHandler::procResource_HTMLIEngine( const std::string & sRealFullPath, Authentication::Multi *extraAuths)
{
    // Drop the MMAP container:
    std::string fileContent;

    if (boost::starts_with(sRealFullPath,"MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids29::Memory::Containers::B_MEM *)getResponseDataStreamer().get())->toString();
        m_serverResponse.setDataStreamer(nullptr);
    }
    else
    {
        m_serverResponse.setDataStreamer(nullptr);
        // Local resource.
        std::ifstream fileStream(sRealFullPath);
        if (!fileStream.is_open())
        {
            log(LEVEL_ERR,"fileServer", 2048, "file not found: %s",sRealFullPath.c_str());
            return HTTP::Status::S_404_NOT_FOUND;
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
        std::ifstream fileIncludeStream(m_resourcesLocalPath + includePath);

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
    jVars["softwareVersion"]   = m_softwareVersion;
    jVars["csrfToken"]         = m_webSession?m_webSession->sCSRFToken:jNull;
    jVars["user"]              = m_authSession?m_authSession->getUserDomainPair().first:jNull;
    jVars["domain"]            = m_authSession?m_authSession->getUserDomainPair().second:jNull;
    jVars["maxAge"]            = (Json::UInt64)(m_webSession?m_sessionMaxAge:0);
    jVars["userAgent"]         = m_clientRequest.userAgent;
    jVars["userIP"]            = m_userIP;
    jVars["userTLSCommonName"] = m_userTLSCommonName;

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
        if ( ! (m_authSession && m_authSession->doesSessionVariableExist(varName))  )
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,m_authSession->getSessionVariableValue(varName),false,scriptVarName);
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
        if (m_clientRequest.getVars(HTTP_VARS_POST)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue(varName));
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
        if (m_clientRequest.getVars(HTTP_VARS_GET)->exist(varName))
        {
            replaceTagByJVar(fileContent,fulltag,m_clientRequest.getVars(HTTP_VARS_GET)->getStringValue(varName));
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

        std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
        procJAPI_Exec(extraAuths,functionName,functionInput, jPayloadOutStr);
        replaceTagByJVar(fileContent,fulltag,*(jPayloadOutStr->getValue()),true,scriptVarName);
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
    if (m_authSession)
        m_authSession->updateLastActivity();

    // Stream the generated content...
    getResponseDataStreamer()->writeString(fileContent);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJAPI_Session()
{
    std::string sMode;
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;

    // GET VARS:
    sMode = m_clientRequest.getVars(HTTP_VARS_GET)->getStringValue("mode");

    // In AUTHCSRF we take the session ID from post variable (not cookie).
    if (m_sessionId.empty() && m_usingCSRFToken && sMode == "AUTHCSRF")
    {
        m_sessionId = m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("sessionId");
        // Open the session again (with the post value):
        sessionOpen();
    }

    // TODO: active check of origin to avoid cross-domain

    /////////////////////////////////////////////////////////////////
    // CHECK CSRF TOKEN HERE.
    // TODO: 1 pre-session per user ;-)
    bool csrfValidationOK = true;
    if (m_usingCSRFToken)
    {
        csrfValidationOK = false;
        if (m_webSession)
        {
            // The login token has not been confirmed yet...
            // AUTHCONFIRM will confirm this token against csrfToken POST data.
            if (!m_webSession->bAuthTokenConfirmed && sMode == "AUTHCSRF")
            {
                if (m_webSession->confirmAuthCSRFToken(m_clientCSRFToken))
                {
                    // Now this method will fixate the introduced session in the browser...
                    log(LEVEL_DEBUG, "apiServer", 2048, "CSRF Confirmation Token OK");
                    m_serverResponse.setSecureCookie("sessionId", m_sessionId, m_sessionMaxAge);
                    eHTTPResponseRetCode = HTTP::Status::S_200_OK;
                }
                else
                {
                    log(LEVEL_ERR,  "apiServer", 2048, "Invalid CSRF Confirmation Token {mode=%s}", sMode.c_str());
                    eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
                }
            }
            // Session found and auth token already confirmed, CSRF token must match session.
            else if (m_webSession->bAuthTokenConfirmed && m_webSession->validateCSRFToken(m_clientCSRFToken))
            {
                log(LEVEL_DEBUG,  "apiServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (m_webSession->bAuthTokenConfirmed && !m_webSession->validateCSRFToken(m_clientCSRFToken) && !(sMode == "CSRFTOKEN"))
            {
                log(LEVEL_ERR,  "apiServer", 2048, "Invalid CSRF Token {mode=%s}", sMode.c_str());
            }
            // We are just going to obtain the CSRF Token
            else if (m_webSession->bAuthTokenConfirmed && (sMode == "CSRFTOKEN"))
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
        if ( !m_webSession && sMode == "LOGIN" && (!m_usingCSRFToken || m_clientCSRFToken == "00112233445566778899"))
            eHTTPResponseRetCode = procJAPI_Session_LOGIN(m_credentials);
        /////////////////////////////////////////////////////////////////
        // POST PERSISTENT AUTHENTICATION...
        else if (m_webSession && m_webSession->bAuthTokenConfirmed && sMode == "POSTLOGIN")
            eHTTPResponseRetCode = procJAPI_Session_POSTLOGIN(m_credentials);
        /////////////////////////////////////////////////////////////////
        // CHANGE MY ACCOUNT PASSWORD...
        else if (m_webSession && m_authSession && // exist a websession+authSession
                 m_webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 m_authSession->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "CHPASSWD") // CHPASSWD command
            eHTTPResponseRetCode = procJAPI_Session_CHPASSWD(m_credentials);
        /////////////////////////////////////////////////////////////////
        // TEST MY ACCOUNT PASSWORD...
        else if (m_webSession && m_authSession && // exist a websession+authSession
                 m_webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 m_authSession->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "TESTPASSWD") // TESTPASSWD command
            eHTTPResponseRetCode = procJAPI_Session_TESTPASSWD(m_credentials);
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        // GET MY ACCOUNT PASSWORD LIST/DESCRIPTION...
        else if (m_webSession && m_authSession && // exist a websession+authSession
                 m_webSession->bAuthTokenConfirmed && // The session is CSRF Confirmed/Obtained
                 m_authSession->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS) && // All required login passwords passed the authorization
                 sMode == "PASSWDLIST") // PASSWDLIST command
            eHTTPResponseRetCode = procJAPI_Session_PASSWDLIST();
        /////////////////////////////////////////////////////////////////
        // CSRF TOKEN REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( m_webSession && m_webSession->bAuthTokenConfirmed && sMode == "CSRFTOKEN" )
            eHTTPResponseRetCode = procJAPI_Session_CSRFTOKEN();
        /////////////////////////////////////////////////////////////////
        // AUTH INFO REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( m_webSession && m_webSession->bAuthTokenConfirmed && sMode == "AUTHINFO" )
            eHTTPResponseRetCode = procJAPI_Session_AUTHINFO();
        /////////////////////////////////////////////////////////////////
        // PERSISTENT SESSION LOGOUT
        else if ( m_webSession && sMode == "LOGOUT" )
        {
            eHTTPResponseRetCode = HTTP::Status::S_200_OK;
            m_destroySession = true;
        }
    }

    // return the HTTP response code.
    return eHTTPResponseRetCode;
}

bool ClientHandler::csrfValidate()
{
    /////////////////////////////////////////////////////////////////
    // CHECK CSRF TOKEN HERE.
    // TODO: 1 pre-session per user ;-)
    bool csrfValidationOK = true;
    if (m_usingCSRFToken)
    {
        csrfValidationOK = false;
        if (m_webSession)
        {
            // The login token has not been confirmed yet...
            // Session found and auth token already confirmed, CSRF token must match session.
            if (m_webSession->bAuthTokenConfirmed && m_webSession->validateCSRFToken(m_clientCSRFToken))
            {
                log(LEVEL_DEBUG, "apiServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (m_webSession->bAuthTokenConfirmed && !m_webSession->validateCSRFToken(m_clientCSRFToken))
            {
                log(LEVEL_ERR, "apiServer", 2048, "Invalid CSRF Token {mode=EXEC}");
            }
        }
    }

    return csrfValidationOK;
}

Status::eRetCode ClientHandler::procResource_File(Authentication::Multi *extraAuths)
{
    // WEB RESOURCE MODE:
    HTTP::Status::eRetCode ret  = HTTP::Status::S_404_NOT_FOUND;
    sLocalRequestedFileInfo fileInfo;
    uint64_t uMaxAge=0;

    // if there are no web resources path, return 404 without data.
    if (m_resourcesLocalPath.empty())
            return HTTP::Status::S_404_NOT_FOUND;

    if ( //staticContent ||
         (getLocalFilePathFromURI2(m_resourcesLocalPath, &fileInfo, ".html") ||
          getLocalFilePathFromURI2(m_resourcesLocalPath, &fileInfo, "index.html") ||
          getLocalFilePathFromURI2(m_resourcesLocalPath, &fileInfo, "")
          ) && !fileInfo.isDir
         )
    {
        // Evaluate...
        API::Web::ResourcesFilter::FilterEvaluationResult e;

        // If there is any session (hSession), then get the authorizer from the session
        // if not, the authorizer is null.
        Mantids29::Authentication::Manager * authorizer = m_authSession?m_authDomains->openDomain(m_authSession->getAuthenticatedDomain()) : nullptr;

        // if there is any resource filter, evaluate the sRealRelativePath with the action to be taken for that file
        // it will proccess this according to the authorization session
        if ( m_resourceFilter )
            e = m_resourceFilter->evaluateURIWithSession(fileInfo.sRealRelativePath, m_authSession, authorizer);

        // If the element is accepted (during the filter)
        if (e.accept)
        {
            // and there is not redirect's, the resoponse code will be 200 (OK)
            if (e.redirectLocation.empty())
                ret = HTTP::Status::S_200_OK;
            else // otherwise you will need to redirect.
                ret = setResponseRedirect( e.redirectLocation );
        }
        else // If not, drop a 403 (forbidden)
            ret = HTTP::Status::S_403_FORBIDDEN;

        log(LEVEL_DEBUG,"fileServer", 2048, "R/ - LOCAL - %03d: %s",HTTP::Status::getHTTPStatusCodeTranslation(ret),fileInfo.sRealFullPath.c_str());
    }
    else
    {
        // File not found at this point (404)
        log(LEVEL_WARN,"fileServer", 65535, "R/404: %s",m_clientRequest.getURI().c_str());
    }

    if (ret != HTTP::Status::S_200_OK)
    {
        // For NON-200 responses, will stream nothing....
        m_serverResponse.setDataStreamer(nullptr);
    }

    // If the URL is going to process the Interactive HTML Engine,
    // and the document content is text/html, then, process it as HTMLIEngine:
    if ( m_useHTMLIEngine && m_serverResponse.contentType == "text/html" ) // The content type has changed during the map.
        ret = procResource_HTMLIEngine(fileInfo.sRealFullPath,extraAuths);

    // And if the file is not found and there are redirections, set the redirection:
    if (ret==HTTP::Status::S_404_NOT_FOUND && !m_redirectPathOn404.empty())
    {
        ret = setResponseRedirect( m_redirectPathOn404 );
    }

    // Log the response.
    log(ret==HTTP::Status::S_200_OK?LEVEL_INFO:LEVEL_WARN,
        "fileServer", 2048, "R/%03d: %s",
        HTTP::Status::getHTTPStatusCodeTranslation(ret),
        ret==HTTP::Status::S_200_OK?fileInfo.sRealRelativePath.c_str():m_clientRequest.getURI().c_str());

    return ret;
}

Status::eRetCode ClientHandler::procJAPI_Version()
{
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["version"]  = m_softwareVersion;
    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJAPI_Session_AUTHINFO()
{
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);

    (*(jPayloadOutStr->getValue()))["user"]   = !m_authSession?"":m_authSession->getUserDomainPair().first;
    (*(jPayloadOutStr->getValue()))["domain"] = !m_authSession?"":m_authSession->getUserDomainPair().second;
    (*(jPayloadOutStr->getValue()))["maxAge"] = (Json::UInt64)m_sessionMaxAge;

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJAPI_Session_CSRFTOKEN()
{
    // On Page Load...
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["csrfToken"] = m_webSession->sCSRFToken;
    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);

    // Update last activity on each page load.
    if (m_authSession)
        m_authSession->updateLastActivity();

    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJAPI_Session_LOGIN(const Authentication::Data & auth)
{
    Mantids29::Authentication::Reason authReason;
    uint64_t uMaxAge;
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;


    std::string user = m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("user");
    std::string domain = m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("domain");

    // Authenticate...
    m_sessionId = persistentAuthentication( user,
                                           domain,
                                           auth,
                                           nullptr, &authReason);


    (*(jPayloadOutStr->getValue()))["txt"] = getReasonText(authReason);
    (*(jPayloadOutStr->getValue()))["val"] = static_cast<Json::UInt>(authReason);

    // Set the parameters once, the first time we see sessionid...
    if (!m_sessionId.empty())
    {
        WebSession * currentWebSession = m_sessionsManager->openSession(m_sessionId,&uMaxAge);
        if (currentWebSession)
        {
            if (!m_usingCSRFToken)
            {
                // If not using CSRF Token, the session id will be fixated in the session cookie...
                m_serverResponse.setSecureCookie("sessionId", m_sessionId, uMaxAge);
            }
            else
            {
                // If using CSRF Token, pass the session id by JSON, because this session should not be fixated in the browser
                (*(jPayloadOutStr->getValue()))["sessionId"] = m_sessionId;
                (*(jPayloadOutStr->getValue()))["maxAge"] = (Json::UInt64)uMaxAge;
            }
            // The session is openned, the CSRF token should be confirmed...
            (*(jPayloadOutStr->getValue()))["csrfAuthConfirm"] = currentWebSession->sCSRFAuthConfirmToken;

            (*(jPayloadOutStr->getValue()))["nextPassReq"] = false;
            auto i = currentWebSession->authSession->getNextRequiredAuthenticationIndex();
            if (i.first != 0xFFFFFFFF)
            {
                // No next login idx.
                (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
                (*(jPayloadOutStr->getValue()))["nextPassReq"]["idx"] = i.first;
                (*(jPayloadOutStr->getValue()))["nextPassReq"]["desc"] = i.second;

                log(LEVEL_INFO,  "apiServer", 2048, "Logged in, waiting for the next authentication factor {val=%d,txt=%s}",
                    JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }
            else
            {
                log(LEVEL_INFO,  "apiServer", 2048, "Logged in {val=%d,txt=%s}", JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""));
            }

            eHTTPResponseRetCode = HTTP::Status::S_200_OK;

            m_sessionsManager->releaseSession(m_sessionId);
        }
    }
    else
    {
        // TODO: for better log, remove usage of , in user/domain
        log(LEVEL_WARN,  "apiServer", 2048, "Invalid Login Attempt {val=%d,txt=%s,user=%s,domain=%s}",
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0),
            JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt",""),
            user.c_str(),
            domain.c_str());

    }

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return eHTTPResponseRetCode;
}

Status::eRetCode ClientHandler::procJAPI_Session_POSTLOGIN(const Authentication::Data &auth)
{
    Mantids29::Authentication::Reason authReason;
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    // Authenticate...
    // We fill the sSessionId in case we want to destroy it with bDestroySession
    m_sessionId = persistentAuthentication( m_authSession->getAuthUser(),
                                           m_authSession->getAuthenticatedDomain(),
                                           auth,
                                           m_authSession, &authReason);

    (*(jPayloadOutStr->getValue()))["txt"] = getReasonText(authReason);
    (*(jPayloadOutStr->getValue()))["val"] = static_cast<Json::UInt>(authReason);

    (*(jPayloadOutStr->getValue()))["nextPassReq"] = false;

    // If the password is authenticated, proceed to report the next required pass:
    if ( IS_PASSWORD_AUTHENTICATED(authReason) )
    {
        auto i = m_authSession->getNextRequiredAuthenticationIndex();
        if (i.first != 0xFFFFFFFF)
        {
            // No next login idx.
            (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["idx"] = i.first;
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["desc"] = i.second;

            log(LEVEL_INFO,  "apiServer", 2048, "Authentication factor (%d) OK, waiting for the next authentication factor {val=%d,txt=%s}", auth.m_passwordIndex, i.first, i.second.c_str());
        }
        else
        {
            log(LEVEL_INFO,  "apiServer", 2048, "Authentication factor (%d) OK, Logged in.", auth.m_passwordIndex);
        }
        eHTTPResponseRetCode = HTTP::Status::S_200_OK;
    }
    else
    {
        log(LEVEL_WARN,  "apiServer", 2048, "Authentication error on factor #(%d), Logged out {val=%d,txt=%s}",auth.m_passwordIndex,
            JSON_ASUINT((*(jPayloadOutStr->getValue())),"val",0), JSON_ASCSTRING((*(jPayloadOutStr->getValue())),"txt","")
            );

        // Mark to Destroy the session if the chpasswd is invalid...
        m_destroySession = true;
        eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
    }

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return eHTTPResponseRetCode;
}

Status::eRetCode ClientHandler::procJAPI_Exec(Authentication::Multi *extraAuths,
                                                     std::string sMethodName,
                                                     std::string sPayloadIn,
                                                     std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr
                                                     )
{
    bool useExternalPayload = !jPayloadOutStr?false:true;

    // External payloads does not csrf validate. (eg. inline html execution)
    if (!useExternalPayload)
    {
        if (!csrfValidate())
            return HTTP::Status::S_404_NOT_FOUND;

        jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    }

    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;

    json jPayloadIn;
    Mantids29::Helpers::JSONReader2 reader;

    std::string  userName   = m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("user");
    std::string domainName  = m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("domain");

    // If there is a session, overwrite the user/domain inputs...
    if (m_authSession)
    {
        userName = m_authSession->getAuthUser();
        domainName = m_authSession->getAuthenticatedDomain();
    }

    if (!m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("payload").empty() && !reader.parse(sPayloadIn, jPayloadIn))
    {
        log(LEVEL_ERR,  "apiServer", 2048, "Invalid JSON Payload for execution {method=%s}", sMethodName.c_str());
        return HTTP::Status::S_400_BAD_REQUEST;
    }

    if (!m_authDomains)
    {
        log(LEVEL_CRITICAL,  "apiServer", 2048, "No authentication domain list exist.");
        return HTTP::Status::S_500_INTERNAL_SERVER_ERROR;
    }

    if (m_methodsHandler->getMethodRequireFullAuth(sMethodName) && !m_authSession)
    {
        log(LEVEL_ERR, "apiServer", 2048, "This method requires full authentication {method=%s}", sMethodName.c_str());
        // Method not available for this null session..
        return HTTP::Status::S_404_NOT_FOUND;
    }

    // TODO: what happens if we are given with unhandled but valid auths that should not be validated...?
    // Get/Pass the temporary authentications for null and not-null sessions:
    std::set<uint32_t> extraTmpIndexes;
    for (const uint32_t & passIdx : extraAuths->getAvailableIndices())
    {
        Mantids29::Authentication::Reason authReason=temporaryAuthentication( userName,
                                                                            domainName,
                                                                            extraAuths->getAuthentication(passIdx) );

        // Include the pass idx in the Extra TMP Index.
        if ( Mantids29::Authentication::IS_PASSWORD_AUTHENTICATED( authReason ) )
        {
            log(LEVEL_INFO, "apiServer", 2048, "Adding valid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids29::Authentication::getReasonText(authReason));
            extraTmpIndexes.insert(passIdx);
        }
        else
        {
            log(LEVEL_WARN, "apiServer", 2048, "Rejecting invalid in-execution authentication factor {method=%s,idx=%d,reason=%s}", sMethodName.c_str(),passIdx,Mantids29::Authentication::getReasonText(authReason));
        }
    }

    auto authorizer = m_authDomains->openDomain(domainName);
    if (authorizer)
    {
        json reasons;

        // Validate that the RPC method is ready to go (fully authorized and no password is expired).
        auto i = m_methodsHandler->validatePermissions( authorizer,  m_authSession, sMethodName, extraTmpIndexes, &reasons );

        m_authDomains->releaseDomain(domainName);

        switch (i)
        {
        case API::Monolith::MethodsHandler::VALIDATION_OK:
        {
            if (m_authSession)
                m_authSession->updateLastActivity();

            log(LEVEL_INFO, "apiServer", 2048, "Executing Web Method {method=%s}", sMethodName.c_str());
            log(LEVEL_DEBUG, "apiServer", 8192, "Executing Web Method - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids29::Helpers::jsonToString(jPayloadIn).c_str());

            auto start = chrono::high_resolution_clock::now();
            auto finish = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = finish - start;

            switch (m_methodsHandler->invoke( m_authDomains,domainName, m_authSession, sMethodName, jPayloadIn, jPayloadOutStr->getValue()))
            {
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

                finish = chrono::high_resolution_clock::now();
                elapsed = finish - start;

                log(LEVEL_INFO, "apiServer", 2048, "Web Method executed OK {method=%s, elapsedMS=%f}", sMethodName.c_str(),elapsed.count());
                log(LEVEL_DEBUG, "apiServer", 8192, "Web Method executed OK - debugging parameters {method=%s,params=%s}", sMethodName.c_str(),Mantids29::Helpers::jsonToString(jPayloadOutStr->getValue()).c_str());

                eHTTPResponseRetCode = HTTP::Status::S_200_OK;
                break;
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:
                log(LEVEL_ERR, "apiServer", 2048, "Web Method not found {method=%s}", sMethodName.c_str());
                eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
                break;
            case API::Monolith::MethodsHandler::METHOD_RET_CODE_INVALIDDOMAIN:
                // This code should never be executed... <
                log(LEVEL_ERR, "apiServer", 2048, "Domain not found during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
                break;
            default:
                log(LEVEL_ERR, "apiServer", 2048, "Unknown error during web method execution {method=%s}", sMethodName.c_str());
                eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
                break;
            }
        }break;
        case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
            log(LEVEL_ERR, "apiServer", 8192, "Not authorized to execute method {method=%s,reasons=%s}", sMethodName.c_str(),Mantids29::Helpers::jsonToString(reasons).c_str());
            eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
        }break;
        case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
        default:
        {
            log(LEVEL_ERR, "apiServer", 2048, "Method not found {method=%s}", sMethodName.c_str());
            // not enough permissions.
            eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
        }break;
        }
    }
    else
    {
        log(LEVEL_ERR, "apiServer", 2048, "Domain not found {method=%s}", sMethodName.c_str());

        // Domain Not found.
        eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
    }

    if (!useExternalPayload)
    {
        m_serverResponse.setDataStreamer(jPayloadOutStr);
        m_serverResponse.setContentType("application/json",true);
    }
    return eHTTPResponseRetCode;
}

Status::eRetCode ClientHandler::procJAPI_Session_CHPASSWD(const Authentication::Data &oldAuth)
{
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    if (!m_authSession)
        return eHTTPResponseRetCode;

    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);

    Authentication::Data newAuth;

    // POST VARS / AUTH:
    if (!newAuth.setJsonString(m_clientRequest.getVars(HTTP_VARS_POST)->getStringValue("newAuth")))
    {
        log(LEVEL_ERR, "apiServer", 2048, "Invalid JSON Parsing for new credentials item");
        return HTTP::Status::S_400_BAD_REQUEST;
    }

    if (oldAuth.m_passwordIndex!=newAuth.m_passwordIndex)
    {
        log(LEVEL_ERR, "apiServer", 2048, "Provided credential index differs from new credential index.");
        return HTTP::Status::S_400_BAD_REQUEST;
    }

    uint32_t credIdx = newAuth.m_passwordIndex;

    auto domainAuthenticator = m_authDomains->openDomain(m_authSession->getAuthenticatedDomain());
    if (domainAuthenticator)
    {
        Mantids29::Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = m_userIP;
        clientDetails.tlsCommonName = m_userTLSCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;

        auto authReason = domainAuthenticator->authenticate(m_applicationName,clientDetails,m_authSession->getAuthUser(),oldAuth.m_password,credIdx);

        if (IS_PASSWORD_AUTHENTICATED(authReason))
        {
            // TODO: alternative/configurable password storage...
            // TODO: check password policy.
            Mantids29::Authentication::Secret newSecretData = Mantids29::Authentication::createNewSecret(newAuth.m_password,Mantids29::Authentication::FN_SSHA256);

            (*(jPayloadOutStr->getValue()))["ok"] = domainAuthenticator->accountChangeAuthenticatedSecret(m_applicationName,
                                                                                                          m_authSession->getAuthUser(),
                                                                                                          credIdx,
                                                                                                          oldAuth.m_password,
                                                                                                          newSecretData,
                                                                                                          clientDetails
                                                                                                          );

            if ( JSON_ASBOOL((*(jPayloadOutStr->getValue())),"ok",false) == true)
                log(LEVEL_INFO, "apiServer", 2048, "Password change requested {index=%d,result=1}",credIdx);
            else
                log(LEVEL_ERR, "apiServer", 2048, "Password change failed due to internal error {index=%d,result=0}",credIdx);

            eHTTPResponseRetCode = HTTP::Status::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, "apiServer", 2048, "Password change failed, bad incomming credentials {index=%d,reason=%s}",credIdx,Mantids29::Authentication::getReasonText(authReason));

            // Mark to Destroy the session if the chpasswd is invalid...
            m_destroySession = true;
            eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
        }


        m_authDomains->releaseDomain(m_authSession->getAuthenticatedDomain());
    }
    else
    {
        log(LEVEL_ERR, "apiServer", 2048, "Password change failed, domain authenticator not found {index=%d}",credIdx);
    }

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return eHTTPResponseRetCode;

}

Status::eRetCode ClientHandler::procJAPI_Session_TESTPASSWD(const Authentication::Data &auth)
{
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    if (!m_authSession) return eHTTPResponseRetCode;

    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);

    auto domainAuthenticator = m_authDomains->openDomain(m_authSession->getAuthenticatedDomain());
    if (domainAuthenticator)
    {
        uint32_t credIdx = auth.m_passwordIndex;

        Mantids29::Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = m_userIP;
        clientDetails.tlsCommonName = m_userTLSCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;

        auto authReason = domainAuthenticator->authenticate(m_applicationName,clientDetails,m_authSession->getAuthUser(),auth.m_password,credIdx);
        if (IS_PASSWORD_AUTHENTICATED(authReason))
        {
            log(LEVEL_INFO, "apiServer", 2048, "Password validation requested {index=%d,result=1}",auth.m_passwordIndex);
            //(*(jPayloadOutStr->getValue()))["ok"] = true;
            eHTTPResponseRetCode = HTTP::Status::S_200_OK;
        }
        else
        {
            log(LEVEL_ERR, "apiServer", 2048, "Password validation failed, bad incomming credentials {index=%d,reason=%s}",auth.m_passwordIndex,Mantids29::Authentication::getReasonText(authReason));

            // Mark to destroy the session if the test password is invalid...
            m_destroySession = true;
            eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
        }
        (*(jPayloadOutStr->getValue()))["ok"] = true;


        m_authDomains->releaseDomain(m_authSession->getAuthenticatedDomain());
    }
    else
    {
        log(LEVEL_ERR, "apiServer", 2048, "Password validation failed, domain authenticator not found {index=%d}",auth.m_passwordIndex);
    }

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return eHTTPResponseRetCode;
}

Status::eRetCode ClientHandler::procJAPI_Session_PASSWDLIST()
{
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    if (!m_authSession) return eHTTPResponseRetCode;

    eHTTPResponseRetCode = HTTP::Status::S_200_OK;
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);

    auto domainAuthenticator = m_authDomains->openDomain(m_authSession->getAuthenticatedDomain());
    if (domainAuthenticator)
    {
        std::map<uint32_t, Mantids29::Authentication::Secret_PublicData> publics = domainAuthenticator->getAccountAllSecretsPublicData(m_authSession->getAuthUser());

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
        eHTTPResponseRetCode = HTTP::Status::S_500_INTERNAL_SERVER_ERROR;

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return eHTTPResponseRetCode;
}

void ClientHandler::setSessionsManagger(SessionsManager *value)
{
    m_sessionsManager = value;
}

void ClientHandler::setUserIP(const std::string &value)
{
    m_userIP = value;
}

// TODO: pasar a Session
std::string ClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Authentication::Data &authData, Mantids29::Authentication::Session *lAuthSession, Mantids29::Authentication::Reason * authReason)
{
    json payload;
    std::string sessionId;
    std::map<uint32_t,std::string> stAccountPassIndexesUsedForLogin;

    // Don't allow other than 0 idx in the first auth. (Return empty session ID with internal error.)
    if (!lAuthSession && authData.m_passwordIndex!=0)
    {
        *authReason = Mantids29::Authentication::REASON_INTERNAL_ERROR;
        return sessionId;
    }

    // Next, if the requested domain is not valid,
    *authReason = Mantids29::Authentication::REASON_INVALID_DOMAIN;

    auto domainAuthenticator = m_authDomains->openDomain(domainName);
    if (domainAuthenticator)
    {

        Mantids29::Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = m_userIP;
        clientDetails.tlsCommonName = m_userTLSCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;

        *authReason = domainAuthenticator->authenticate(m_applicationName,clientDetails,userName,authData.m_password,authData.m_passwordIndex, Mantids29::Authentication::MODE_PLAIN,"",&stAccountPassIndexesUsedForLogin);

        m_authDomains->releaseDomain(domainName);
    }

    if ( Mantids29::Authentication::IS_PASSWORD_AUTHENTICATED( *authReason ) )
    {
        // If not exist an authenticated session, create a new one.
        if (!lAuthSession)
        {
            lAuthSession = new Mantids29::Authentication::Session(m_applicationName);
            lAuthSession->setPersistentSession(true);
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.m_passwordIndex,*authReason);

            // The first pass/time the list of idx should be filled into.
            if (authData.m_passwordIndex==0)
                lAuthSession->setRequiredBasicAuthenticationIndices(stAccountPassIndexesUsedForLogin);

            // Add to session manager (creates web session).
            sessionId = m_sessionsManager->createWebSession(lAuthSession);

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
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.m_passwordIndex,*authReason);
            sessionId = lAuthSession->getSessionId();
        }
    }

    return sessionId;
}

Mantids29::Authentication::Reason ClientHandler::temporaryAuthentication(const std::string & userName, const std::string & domainName, const Authentication::Data &authData)
{
    Mantids29::Authentication::Reason eReason;

    auto auth = m_authDomains->openDomain(domainName);
    if (!auth)
        eReason = Mantids29::Authentication::REASON_INVALID_DOMAIN;
    else
    {
        Mantids29::Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = m_userIP;
        clientDetails.tlsCommonName = m_userTLSCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;

        eReason = auth->authenticate( m_applicationName, clientDetails, userName,authData.m_password,authData.m_passwordIndex); // Authenticate in a non-persistent fashion.
        m_authDomains->releaseDomain(domainName);
    }

    return eReason;
}

void ClientHandler::log(eLogLevels logSeverity,  const std::string & module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);

    if (m_rpcLog) m_rpcLog->logVA( logSeverity,
                               m_userIP,
                               !m_authSession?"" : m_authSession->getSessionId(),
                               !m_authSession?"" : m_authSession->getAuthUser(),
                               !m_authSession?"" : m_authSession->getAuthenticatedDomain(),
                               module, outSize,fmtLog,args);

    va_end(args);
}

void ClientHandler::setRedirectPathOn404(const std::string &newRedirectOn404)
{
    m_redirectPathOn404 = newRedirectOn404;
}

void ClientHandler::setRPCLog(Program::Logs::RPCLog *value)
{
    m_rpcLog = value;
}


void ClientHandler::setRemoteTLSCN(const std::string &value)
{
    m_userTLSCommonName = value;
}

std::string ClientHandler::getApplicationName() const
{
    return m_applicationName;
}

void ClientHandler::setAppName(const std::string &value)
{
    m_applicationName = value;
}

void ClientHandler::setUseHTMLIEngine(bool value)
{
    m_useHTMLIEngine = value;
}

void ClientHandler::setSoftwareVersion(const std::string &value)
{
    m_softwareVersion = value;
}

void ClientHandler::setWebServerName(const std::string &value)
{
    m_webServerName = value;
    if (!m_webServerName.empty())
    {
        setResponseServerName(m_webServerName);
    }
}

void ClientHandler::setUsingCSRFToken(bool value)
{
    m_usingCSRFToken = value;
}

void ClientHandler::setDocumentRootPath(const std::string &value)
{
    if (value.empty())
    {
        m_resourcesLocalPath = "";
        return;
    }

    char * cFullPath = realpath(value.c_str(), nullptr);
    if (cFullPath)
    {
        m_resourcesLocalPath = cFullPath;
        free(cFullPath);
    }
    else
        m_resourcesLocalPath = value;
}

void ClientHandler::setResourcesFilter(API::Monolith::ResourcesFilter *value)
{
    m_resourceFilter = value;
}

void ClientHandler::setUseFormattedJSONOutput(bool value)
{
    m_useFormattedJSONOutput = value;
}

void ClientHandler::setMethodsHandler(API::Monolith::MethodsHandler *value)
{
    m_methodsHandler = value;
}



