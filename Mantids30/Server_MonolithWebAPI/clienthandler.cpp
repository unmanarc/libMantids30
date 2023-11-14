#include "clienthandler.h"
#include <Mantids30/Auth/credentialdata.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/API_Monolith/methodshandler.h>

#include <Mantids30/Memory/b_mmap.h>
#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/json.h>

#include <memory>
#include <stdarg.h>
#include <fstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>

#ifdef _WIN32
#include <stdlib.h>
// TODO: check if _fullpath mitigate transversal.
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30;
using namespace std;

ClientHandler::ClientHandler(void *parent, Memory::Streams::StreamableObject *sock) : HTTPv1_Server(sock)
{
    // TODO: rpc logs?
    m_resourceFilter = nullptr;
}

ClientHandler::~ClientHandler()
{
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
/*    if (!m_extraCredentials.parseJSON(m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("extraAuth")))
        return HTTP::Status::S_400_BAD_REQUEST;*/

    // POST VARS / AUTH:
    /*if (!m_credentials.parseJSON(m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("auth")))
        return HTTP::Status::S_400_BAD_REQUEST;*/

    // OPEN THE SESSION HERE:
    sessionOpen();

    std::string requestURI = m_clientRequest.getURI();

    // Detect if is /api/v1/jexec, then process the JSON Call Request.

    auto jsonStreamContent = m_clientRequest.getJSONStreamerContent();
    json empty;
    json * jPayloadIn = jsonStreamContent!=nullptr? jsonStreamContent->getValue():&empty;

    //m_clientRequest.getVars()
    //m_clientRequest.getVars(HTTP_VARS_POST)->getVarsAsJSONMap()
    //m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("payload")
    if (boost::starts_with(requestURI,"/api/v1/execute/")) ret = procJSONWebAPI_Exec(requestURI.substr(17),*jPayloadIn);
    // Detect if is /api/v1/session/..., then process the JSON Call Request.
    else if (boost::starts_with(requestURI,"/api/v1/session/")) ret = procJSONWebAPI_Session(requestURI.substr(16));
    // Detect if is /api/v1/version, then process the JSON Call Request.
    else if (requestURI == "/api/v1/version") ret = procJSONWebAPI_Version();
    // Otherwise, process as web Resource
    else ret = procResource_File();

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
            m_session = m_webSession->authSession;
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
Status::eRetCode ClientHandler::procResource_HTMLIEngine( const std::string & sRealFullPath)
{
    // Drop the MMAP container:
    std::string fileContent;

    if (boost::starts_with(sRealFullPath,"MEM:"))
    {
        // Mem-Static resource.
        fileContent = ((Mantids30::Memory::Containers::B_MEM *)getResponseDataStreamer().get())->toString();
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
    jVars["user"]              = m_session?m_session->getEffectiveUser():jNull;
    jVars["maxAge"]            = (Json::UInt64)(m_webSession?m_sessionMaxAge:0);
    jVars["userAgent"]         = m_clientRequest.userAgent;
    jVars["userIP"]            = m_clientRequest.networkClientInfo.REMOTE_ADDR;
    jVars["userTLSCommonName"] = m_clientRequest.networkClientInfo.tlsCommonName;

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
        if ( ! (m_session && m_session->doesSessionVariableExist(varName))  )
        {
            // look in post/get
            log(LEVEL_ERR, "fileserver", 2048, "Main variable not found: '%s' on resource '%s'",varName.c_str(),sRealFullPath.c_str());
            boost::replace_all(fileContent,fulltag, "null");
        }
        else
        {
            replaceTagByJVar(fileContent,fulltag,m_session->getSessionVariableValue(varName),false,scriptVarName);
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
            replaceTagByJVar(fileContent,fulltag,m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>(varName));
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
            replaceTagByJVar(fileContent,fulltag,m_clientRequest.getVars(HTTP_VARS_GET)->getTValue<std::string>(varName));
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
        procJSONWebAPI_Exec(functionName,functionInput, jPayloadOutStr);
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
    if (m_session)
        m_session->updateLastActivity();

    // Stream the generated content...
    getResponseDataStreamer()->writeString(fileContent);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJSONWebAPI_Session(const std::string & sMethodName)
{
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;

    // In AUTHCSRF we take the session ID from post variable (not cookie).
    if (m_sessionId.empty() && m_usingCSRFToken && sMethodName == "authCsrf")
    {
        m_sessionId = m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("sessionId");
        // Open the session again (with the post value):
        sessionOpen();
    }

    // TODO: active check of origin to avoid cross-domain

    /////////////////////////////////////////////////////////////////
    // CHECK CSRF TOKEN HERE.
    // TODO: 1 pre-session per user ;-)
//    bool csrfValidationOK = true;
    /*if (m_usingCSRFToken)
    {
        csrfValidationOK = false;
        if (m_webSession)
        {
            // We require the CSRF token for... authInfo only?

            // The login token has not been confirmed yet...
            // AUTHCONFIRM will confirm this token against csrfToken POST data.
            if (!m_webSession->bAuthTokenConfirmed && sMethodName == "authCsrf")
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
                    log(LEVEL_ERR,  "apiServer", 2048, "Invalid CSRF Confirmation Token {mode=%s}", sMethodName.c_str());
                    eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
                }
            }
            // Session found and auth token already confirmed, CSRF token must match session.
            else if (m_webSession->bAuthTokenConfirmed && m_webSession->validateCSRFToken(m_clientCSRFToken))
            {
                log(LEVEL_DEBUG,  "apiServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else if (m_webSession->bAuthTokenConfirmed && !m_webSession->validateCSRFToken(m_clientCSRFToken) && !(sMethodName == "CSRFTOKEN"))
            {
                log(LEVEL_ERR,  "apiServer", 2048, "Invalid CSRF Token {mode=%s}", sMethodName.c_str());
            }
            // We are just going to obtain the CSRF Token
            else if (m_webSession->bAuthTokenConfirmed && (sMethodName == "CSRFTOKEN"))
                csrfValidationOK = true;
        }
        else if ( sMethodName == "callback" )
        {
            // Session not found... Allow LOGIN without CSRF validation...
            csrfValidationOK = true;
        }
    }*/

   // if (csrfValidationOK)
    {
        /////////////////////////////////////////////////////////////////

        // TODO: validate origin...
        if ( !m_webSession && sMethodName == "callback" )
            eHTTPResponseRetCode = procJSONWebAPI_Session_LoginCallback();
        /////////////////////////////////////////////////////////////////
        // CSRF TOKEN REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( m_webSession && sMethodName == "csrfToken" )
            eHTTPResponseRetCode = procJSONWebAPI_Session_CSRFTOKEN();
        /////////////////////////////////////////////////////////////////
        // AUTH INFO REQUEST... (REQUIRES A VALID SESSION AND A VALID AUTHCSRF CONFIRMATION)
        else if ( m_webSession && sMethodName == "authInfo" )
            eHTTPResponseRetCode = procJSONWebAPI_Session_AUTHINFO();
        /////////////////////////////////////////////////////////////////
        // PERSISTENT SESSION LOGOUT
        else if ( m_webSession && sMethodName == "logout" )
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
            // Session found, CSRF token must match session.
            if (m_webSession->validateCSRFToken(m_clientCSRFToken))
            {
                log(LEVEL_DEBUG, "apiServer", 2048, "CSRF Token OK");
                csrfValidationOK = true;
            }
            else
            {
                log(LEVEL_ERR, "apiServer", 2048, "Invalid CSRF Token {mode=EXEC}");
            }
        }
    }

    return csrfValidationOK;
}

Status::eRetCode ClientHandler::procResource_File()
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

        // if there is any resource filter, evaluate the sRealRelativePath with the action to be taken for that file
        // it will proccess this according to the authorization session
        if ( m_resourceFilter )
            e = m_resourceFilter->evaluateURIWithSession(fileInfo.sRealRelativePath, m_session);

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
        ret = procResource_HTMLIEngine(fileInfo.sRealFullPath);

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

Status::eRetCode ClientHandler::procJSONWebAPI_Version()
{
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["version"]  = m_softwareVersion;
    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJSONWebAPI_Session_AUTHINFO()
{
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);

    (*(jPayloadOutStr->getValue()))["user"]   = !m_session?"":m_session->getEffectiveUser();
    //(*(jPayloadOutStr->getValue()))["domain"] = !m_authSession?"":m_authSession->getUserDomainPair().second;
    (*(jPayloadOutStr->getValue()))["maxAge"] = (Json::UInt64)m_sessionMaxAge;

    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);
    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJSONWebAPI_Session_CSRFTOKEN()
{
    // On Page Load...
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    (*(jPayloadOutStr->getValue()))["csrfToken"] = m_webSession->sCSRFToken;
    m_serverResponse.setDataStreamer(jPayloadOutStr);
    m_serverResponse.setContentType("application/json",true);

    // Update last activity on each page load.
    if (m_session)
        m_session->updateLastActivity();

    return HTTP::Status::S_200_OK;
}

Status::eRetCode ClientHandler::procJSONWebAPI_Session_LoginCallback()
{
    // INITIAL RECEPTION OF THE JWT AUTHENTICATION...

    Mantids30::Auth::Reason authReason;
    uint64_t uMaxAge;
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    std::string user = m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("user");
    //std::string domain = m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("domain");

    // Authenticate...
    m_sessionId = persistentAuthentication( user,
                                         //  domain,
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
            auto i = currentWebSession->authSession->getNextRequiredAuthenticationSlotIdForLogin();
            if (i.first != 0xFFFFFFFF)
            {
                // No next login slotId.
                (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
                (*(jPayloadOutStr->getValue()))["nextPassReq"]["slotId"] = i.first;
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

Status::eRetCode ClientHandler::procJSONWebAPI_Session_POSTLOGIN(const Auth::CredentialData &auth)
{
    Mantids30::Auth::Reason authReason;
    std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
    jPayloadOutStr->setFormatted(m_useFormattedJSONOutput);
    HTTP::Status::eRetCode eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;

    // Authenticate...
    // We fill the sSessionId in case we want to destroy it with bDestroySession
    m_sessionId = persistentAuthentication( m_session->getAuthUser(),
                               //            m_authSession->getAuthenticatedDomain(),
                                           auth,
                                           m_session, &authReason);

    (*(jPayloadOutStr->getValue()))["txt"] = getReasonText(authReason);
    (*(jPayloadOutStr->getValue()))["val"] = static_cast<Json::UInt>(authReason);

    (*(jPayloadOutStr->getValue()))["nextPassReq"] = false;

    // If the password is authenticated, proceed to report the next required pass:
    if ( IS_PASSWORD_AUTHENTICATED(authReason) )
    {
        auto i = m_session->getNextRequiredAuthenticationSlotIdForLogin();
        if (i.first != 0xFFFFFFFF)
        {
            // No next login slotId.
            (*(jPayloadOutStr->getValue())).removeMember("nextPassReq");
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["slotId"] = i.first;
            (*(jPayloadOutStr->getValue()))["nextPassReq"]["desc"] = i.second;

            log(LEVEL_INFO,  "apiServer", 2048, "Authentication factor (%d) OK, waiting for the next authentication factor {val=%d,txt=%s}", auth.m_slotId, i.first, i.second.c_str());
        }
        else
        {
            log(LEVEL_INFO,  "apiServer", 2048, "Authentication factor (%d) OK, Logged in.", auth.m_slotId);
        }
        eHTTPResponseRetCode = HTTP::Status::S_200_OK;
    }
    else
    {
        log(LEVEL_WARN,  "apiServer", 2048, "Authentication error on factor #(%d), Logged out {val=%d,txt=%s}",auth.m_slotId,
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

Status::eRetCode ClientHandler::procJSONWebAPI_Exec(   std::string sMethodName,
                                                    const json & jPayloadIn,
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

    //json jPayloadIn;
    Mantids30::Helpers::JSONReader2 reader;

//    std::string  userName   = m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("user");
//    std::string domainName  = m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("domain");

    // If there is a session, overwrite the user/domain inputs...
  /*  if (m_authSession)
    {
        userName = m_authSession->getEffectiveUser();
    }*/
/*
    if (!m_clientRequest.getVars(HTTP_VARS_POST)->getTValue<std::string>("payload").empty() && !reader.parse(jPayloadIn, jPayloadIn))
    {
        log(LEVEL_ERR,  "apiServer", 2048, "Invalid JSON Payload for execution {method=%s}", sMethodName.c_str());
        return HTTP::Status::S_400_BAD_REQUEST;
    }*/
/*
    if (!m_authDomains)
    {
        log(LEVEL_CRITICAL,  "apiServer", 2048, "No authentication domain list exist.");
        return HTTP::Status::S_500_INTERNAL_SERVER_ERROR;
    }
    */

    if (m_methodsHandler->doesMethodRequireActiveSession(sMethodName) && !m_session)
    {
        log(LEVEL_ERR, "apiServer", 2048, "This method requires full authentication / session {method=%s}", sMethodName.c_str());
        // Method not available for this null session..
        return HTTP::Status::S_404_NOT_FOUND;
    }

    json reasons;

    // Validate that the method requirements are satisfied.
    auto i = m_methodsHandler->validateMethodRequirements(m_session, sMethodName, &reasons);

    switch (i)
    {
    case API::Monolith::MethodsHandler::VALIDATION_OK:
    {
        if (m_session)
            m_session->updateLastActivity();

        log(LEVEL_INFO, "apiServer", 2048, "Executing Web Method {method=%s}", sMethodName.c_str());
        log(LEVEL_DEBUG, "apiServer", 8192, "Executing Web Method - debugging parameters {method=%s,params=%s}", sMethodName.c_str(), Mantids30::Helpers::jsonToString(jPayloadIn).c_str());

        auto start = chrono::high_resolution_clock::now();
        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = finish - start;

        switch (m_methodsHandler->invoke(m_session, sMethodName, jPayloadIn, jPayloadOutStr->getValue()))
        {
        case API::Monolith::MethodsHandler::METHOD_RET_CODE_SUCCESS:

            finish = chrono::high_resolution_clock::now();
            elapsed = finish - start;

            log(LEVEL_INFO, "apiServer", 2048, "Web Method executed OK {method=%s, elapsedMS=%f}", sMethodName.c_str(), elapsed.count());
            log(LEVEL_DEBUG, "apiServer", 8192, "Web Method executed OK - debugging parameters {method=%s,params=%s}", sMethodName.c_str(), Mantids30::Helpers::jsonToString(jPayloadOutStr->getValue()).c_str());

            eHTTPResponseRetCode = HTTP::Status::S_200_OK;
            break;
        case API::Monolith::MethodsHandler::METHOD_RET_CODE_METHODNOTFOUND:
            log(LEVEL_ERR, "apiServer", 2048, "Web Method not found {method=%s}", sMethodName.c_str());
            eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
            break;
        default:
            log(LEVEL_ERR, "apiServer", 2048, "Unknown error during web method execution {method=%s}", sMethodName.c_str());
            eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
            break;
        }
    }
    break;
    case API::Monolith::MethodsHandler::VALIDATION_NOTAUTHORIZED:
    {
        // not enough permissions.
        (*(jPayloadOutStr->getValue()))["auth"]["reasons"] = reasons;
        log(LEVEL_ERR, "apiServer", 8192, "Not authorized to execute method {method=%s,reasons=%s}", sMethodName.c_str(), Mantids30::Helpers::jsonToString(reasons).c_str());
        eHTTPResponseRetCode = HTTP::Status::S_401_UNAUTHORIZED;
    }
    break;
    case API::Monolith::MethodsHandler::VALIDATION_METHODNOTFOUND:
    default:
    {
        log(LEVEL_ERR, "apiServer", 2048, "Method not found {method=%s}", sMethodName.c_str());
        // not enough permissions.
        eHTTPResponseRetCode = HTTP::Status::S_404_NOT_FOUND;
    }
    break;
    }

    if (!useExternalPayload)
    {
        m_serverResponse.setDataStreamer(jPayloadOutStr);
        m_serverResponse.setContentType("application/json",true);
    }
    return eHTTPResponseRetCode;
}

void ClientHandler::setSessionsManagger(SessionsManager *value)
{
    m_sessionsManager = value;
}

/*
// TODO: pasar a Session
std::string ClientHandler::persistentAuthentication(const string &userName, const string &domainName, const Auth::CredentialData &authData, Mantids30::Auth::Session *lAuthSession, Mantids30::Auth::Reason * authReason)
{
    json payload;
    std::string sessionId;
    std::map<uint32_t,std::string> stAccountAuthenticationSlotsUsedForLogin;

    // Don't allow other than 0 slotId in the first auth. (Return empty session ID with internal error.)
    if (!lAuthSession && authData.m_slotId!=0)
    {
        *authReason = Mantids30::Auth::REASON_INTERNAL_ERROR;
        return sessionId;
    }

    // Next, if the requested domain is not valid,
    *authReason = Mantids30::Auth::REASON_INVALID_DOMAIN;

    auto domainAuthenticator = m_authDomains->openDomain(domainName);
    if (domainAuthenticator)
    {

        Mantids30::Auth::ClientDetails clientDetails;
        clientDetails.ipAddress = m_clientRequest.networkClientInfo.REMOTE_ADDR;
        clientDetails.tlsCommonName = m_clientRequest.networkClientInfo.tlsCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;
        
        *authReason = domainAuthenticator->authController->authenticateCredential(m_applicationName,clientDetails,userName,authData.m_password,authData.m_slotId, Mantids30::Auth::MODE_PLAIN,"",&stAccountAuthenticationSlotsUsedForLogin);

        m_authDomains->releaseDomain(domainName);
    }

    if ( Mantids30::Auth::IS_PASSWORD_AUTHENTICATED( *authReason ) )
    {
        // If not exist an authenticated session, create a new one.
        if (!lAuthSession)
        {
            lAuthSession = new Mantids30::Auth::Session(m_applicationName);
            //lAuthSession->setIsPersistentSession(true);
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.m_slotId,*authReason);

            // The first pass/time the list of slotId should be filled into.
            if (authData.m_slotId==0)
                lAuthSession->setRequiredAuthenticationSlotsForLogin(stAccountAuthenticationSlotsUsedForLogin);

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
            lAuthSession->registerPersistentAuthentication(userName,domainName,authData.m_slotId,*authReason);
            sessionId = lAuthSession->getSessionId();
        }
    }

    return sessionId;
}

Mantids30::Auth::Reason ClientHandler::temporaryAuthentication(const std::string & userName, const std::string & domainName, const Auth::CredentialData &authData)
{
    Mantids30::Auth::Reason eReason;

    auto identityManager = m_authDomains->openDomain(domainName);
    if (!identityManager)
        eReason = Mantids30::Auth::REASON_INVALID_DOMAIN;
    else
    {
        Mantids30::Auth::ClientDetails clientDetails;
        clientDetails.ipAddress = m_clientRequest.networkClientInfo.REMOTE_ADDR;
        clientDetails.tlsCommonName = m_clientRequest.networkClientInfo.tlsCommonName;
        clientDetails.userAgent = m_clientRequest.userAgent;
        
        eReason = identityManager->authController->authenticateCredential( m_applicationName, clientDetails, userName,authData.m_password,authData.m_slotId); // Authenticate in a non-persistent fashion.
        m_authDomains->releaseDomain(domainName);
    }

    return eReason;
}*/

void ClientHandler::log(eLogLevels logSeverity,  const std::string & module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);

    // TODO: log impersonations...
    if (m_rpcLog) m_rpcLog->logVA( logSeverity,
                               m_clientRequest.networkClientInfo.REMOTE_ADDR,
                        !m_session?"" : m_session->getSessionId(),
                        !m_session?"" : m_session->getEffectiveUser(),
                               "",
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



