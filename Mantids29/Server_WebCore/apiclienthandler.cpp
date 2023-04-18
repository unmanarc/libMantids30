#include "apiclienthandler.h"
#include "htmliengine.h"

#include <Mantids29/Auth/data.h>
#include <Mantids29/Protocol_HTTP/rsp_status.h>
#include <Mantids29/Memory/b_mmap.h>
#include <Mantids29/Helpers/crypto.h>
#include <Mantids29/Helpers/json.h>

#include <boost/algorithm/string/predicate.hpp>
#include <stdarg.h>

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
using namespace Mantids29::Network::Servers::Web;
using namespace Mantids29;
using namespace std;

APIClientHandler::APIClientHandler(void *parent, Memory::Streams::StreamableObject *sock) : HTTPv1_Server(sock)
{
}

APIClientHandler::~APIClientHandler()
{
}

Status::eRetCode APIClientHandler::procHTTPClientContent()
{
    HTTP::Status::eRetCode ret  = HTTP::Status::S_404_NOT_FOUND;

    HTTP::Status::eRetCode rtmp;
    if ((rtmp = sessionStart()) != HTTP::Status::S_200_OK)
    {
        return rtmp;
    }

    std::string requestURI = m_clientRequest.getURI();
    bool isAPIURI = false;

    for ( const auto & apiURL : m_APIURLs)
    {
        if (requestURI == apiURL || boost::starts_with(requestURI,apiURL + "/"))
        {
            // It's an API Request.
            ret = handleAPIRequest(apiURL,requestURI == apiURL? "" : requestURI.substr(apiURL.size()+1) );
            isAPIURI = true;
            break;
        }
    }

    if ( !isAPIURI )
    {
        // It's a file request.
        ret = handleFileRequest();
    }

    if ((rtmp = sessionCleanup()) != HTTP::Status::S_200_OK)
    {
        return rtmp;
    }

    return ret;
}

void APIClientHandler::fillUserDataVars(json &jVars)
{
    jVars["user"]              = m_userData.userName;
    jVars["domain"]            = m_userData.domainName;
    jVars["userTLSCommonName"] = m_userData.tlsCommonName;
    jVars["domain"]            = m_userData.domainName;
    jVars["loggedIn"]          = m_userData.loggedIn;
    jVars["sessionActive"]     = m_userData.sessionActive;
    jVars["userIP"]            = m_userData.ipAddress;
}

Status::eRetCode APIClientHandler::handleFileRequest()
{
    // WEB RESOURCE MODE:
    HTTP::Status::eRetCode ret  = HTTP::Status::S_404_NOT_FOUND;
    sLocalRequestedFileInfo fileInfo;
    uint64_t uMaxAge=0;

    // if there are no web resources path, return 404 without data.
    if (m_config.resourcesLocalPath.empty())
            return HTTP::Status::S_404_NOT_FOUND;

    if ( //staticContent ||
         (getLocalFilePathFromURI2(m_config.resourcesLocalPath, &fileInfo, ".html") ||
          getLocalFilePathFromURI2(m_config.resourcesLocalPath, &fileInfo, "index.html") ||
          getLocalFilePathFromURI2(m_config.resourcesLocalPath, &fileInfo, "")
          ) && !fileInfo.isDir
         )
    {
        // Evaluate...
        API::Web::ResourcesFilter::FilterEvaluationResult e;

        // if there is any resource filter, evaluate the sRealRelativePath with the action to be taken for that file
        // it will proccess this according to the authorization session
        if ( m_config.resourceFilter )
            e = m_config.resourceFilter->evaluateURI(fileInfo.sRealRelativePath, &m_userData);

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
        m_serverResponse.setDataStreamer(nullptr,false);
    }

    // If the URL is going to process the Interactive HTML Engine,
    // and the document content is text/html, then, process it as HTMLIEngine:
    if ( m_config.useHTMLIEngine && m_serverResponse.contentType == "text/html" ) // The content type has changed during the map.
    {
        ret = HTMLIEngine::processResourceFile( this, fileInfo.sRealFullPath );
    }

    // And if the file is not found and there are redirections, set the redirection:
    if (ret==HTTP::Status::S_404_NOT_FOUND && !m_config.redirectPathOn404.empty())
    {
        ret = setResponseRedirect( m_config.redirectPathOn404 );
    }

    // Log the response.
    log(ret==HTTP::Status::S_200_OK?LEVEL_INFO:LEVEL_WARN,
        "fileServer", 2048, "R/%03d: %s",
        HTTP::Status::getHTTPStatusCodeTranslation(ret),
        ret==HTTP::Status::S_200_OK?fileInfo.sRealRelativePath.c_str():m_clientRequest.getURI().c_str());

    return ret;
}

void APIClientHandler::setUserIP(const std::string &value)
{
    m_userData.ipAddress = value;
}

void APIClientHandler::log(eLogLevels logSeverity,  const std::string & module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);

    if (m_rpcLog) m_rpcLog->logVA( logSeverity,
                               m_userData.ipAddress,
                               m_userData.halfSessionId,
                               m_userData.userName,
                               m_userData.domainName,
                               module, outSize,fmtLog,args);

    va_end(args);
}

void APIClientHandler::setRedirectPathOn404(const std::string &newRedirectOn404)
{
    m_config.redirectPathOn404 = newRedirectOn404;
}

void APIClientHandler::setRPCLog(Program::Logs::RPCLog *value)
{
    m_rpcLog = value;
}

void APIClientHandler::setRemoteTLSCN(const std::string &value)
{
    m_userData.tlsCommonName = value;
}

std::string APIClientHandler::getApplicationName() const
{
    return m_config.applicationName;
}

void APIClientHandler::setAppName(const std::string &value)
{
    m_config.applicationName = value;
}

void APIClientHandler::setUseHTMLIEngine(bool value)
{
    m_config.useHTMLIEngine = value;
}

void APIClientHandler::setSoftwareVersion(const std::string &value)
{
    m_config.softwareVersion = value;
}

void APIClientHandler::setWebServerName(const std::string &value)
{
    m_config.webServerName = value;
    if (!m_config.webServerName.empty())
    {
        setResponseServerName(m_config.webServerName);
    }
}

void APIClientHandler::setDocumentRootPath(const std::string &value)
{
    if (value.empty())
    {
        m_config.resourcesLocalPath = "";
        return;
    }

    char * cFullPath = realpath(value.c_str(), nullptr);
    if (cFullPath)
    {
        m_config.resourcesLocalPath = cFullPath;
        free(cFullPath);
    }
    else
        m_config.resourcesLocalPath = value;
}

void APIClientHandler::setResourcesFilter(API::Web::ResourcesFilter *value)
{
    m_config.resourceFilter = value;
}

void APIClientHandler::setUseFormattedJSONOutput(bool value)
{
    m_config.useFormattedJSONOutput = value;
}



