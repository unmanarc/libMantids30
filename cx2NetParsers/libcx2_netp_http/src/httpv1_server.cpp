#include "httpv1_server.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <cx2_mem_vars/b_mmap.h>

#include <sys/stat.h>

#ifdef WIN32
#include <stdlib.h>
// TODO: check if _fullpath mitigate transversal.
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2;

HTTPv1_Server::HTTPv1_Server(Memory::Streams::Streamable *sobject) : HTTPv1_Base(false, sobject)
{
    badAnswer = false;
    remotePairAddress[0]=0;
    currentParser = (Memory::Streams::Parsing::SubParser *)(&_clientRequest);

    // Default Mime Types (ref: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types)
    mimeTypes[".aac"] = "audio/aac";
    mimeTypes[".abw"] = "application/x-abiword";
    mimeTypes[".arc"] = "application/x-freearc";
    mimeTypes[".avi"] = "video/x-msvideo";
    mimeTypes[".azw"] = "application/vnd.amazon.ebook";
    mimeTypes[".bin"] = "application/octet-stream";
    mimeTypes[".bmp"] = "image/bmp";
    mimeTypes[".bz"] = "application/x-bzip";
    mimeTypes[".bz2"] = "application/x-bzip2";
    mimeTypes[".csh"] = "application/x-csh";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".csv"] = "text/csv";
    mimeTypes[".doc"] = "application/msword";
    mimeTypes[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mimeTypes[".eot"] = "application/vnd.ms-fontobject";
    mimeTypes[".epub"] = "application/epub+zip";
    mimeTypes[".gz"] = "application/gzip";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".html"] = "text/html";
    mimeTypes[".ico"] = "image/vnd.microsoft.icon";
    mimeTypes[".ics"] = "text/calendar";
    mimeTypes[".jar"] = "application/java-archive";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".jsonld"] = "application/ld+json";
    mimeTypes[".mid"] = "audio/midi";
    mimeTypes[".midi"] = "audio/x-midi";
    mimeTypes[".mjs"] = "text/javascript";
    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".mpeg"] = "video/mpeg";
    mimeTypes[".mpkg"] = "application/vnd.apple.installer+xml";
    mimeTypes[".odp"] = "application/vnd.oasis.opendocument.presentation";
    mimeTypes[".ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    mimeTypes[".odt"] = "application/vnd.oasis.opendocument.text";
    mimeTypes[".oga"] = "audio/ogg";
    mimeTypes[".ogv"] = "video/ogg";
    mimeTypes[".ogx"] = "application/ogg";
    mimeTypes[".opus"] = "audio/opus";
    mimeTypes[".otf"] = "font/otf";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".php"] = "application/x-httpd-php";
    mimeTypes[".ppt"] = "application/vnd.ms-powerpoint";
    mimeTypes[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    mimeTypes[".rar"] = "application/vnd.rar";
    mimeTypes[".rtf"] = "application/rtf";
    mimeTypes[".sh"] = "application/x-sh";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".swf"] = "application/x-shockwave-flash";
    mimeTypes[".tar"] = "application/x-tar";
    mimeTypes[".tif"] = "image/tiff";
    mimeTypes[".ts"] = "video/mp2t";
    mimeTypes[".ttf"] = "font/ttf";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".vsd"] = "application/vnd.visio";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".weba"] = "audio/webm";
    mimeTypes[".webm"] = "video/webm";
    mimeTypes[".webp"] = "image/webp";
    mimeTypes[".woff"] = "font/woff";
    mimeTypes[".woff2"] = "font/woff2";
    mimeTypes[".xhtml"] = "application/xhtml+xml";
    mimeTypes[".xls"] = "application/vnd.ms-excel";
    mimeTypes[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".xul"] = "application/vnd.mozilla.xul+xml";
    mimeTypes[".zip"] = "application/zip";
    mimeTypes[".3gp"] = "video/3gpp</code><br>";
    mimeTypes[".3g2"] = "video/3gpp2</code><br>";
    mimeTypes[".7z"] = "application/x-7z-compressed";

    includeServerDate = true;
}

sHTTP_RequestData HTTPv1_Server::requestData()
{
    sHTTP_RequestData fullReq;

    if (_clientHeaders.exist("Authentication"))
    {
        // TODO: deal with http based authorization.
    }
    if (_clientHeaders.exist("User-Agent"))
    {
        fullReq.USER_AGENT = _clientHeaders.getOptionRawStringByName("User-Agent");
    }

    fullReq.VARS_GET = getRequestVars(HTTP_VARS_GET);
    fullReq.VARS_POST = getRequestVars(HTTP_VARS_POST);
    fullReq.VARS_COOKIES = _clientHeaders.getOptionByName("Cookie");

    fullReq.CLIENT_IP = remotePairAddress;
    fullReq.VIRTUAL_HOST = &virtualHost;
    fullReq.VIRTUAL_PORT = &virtualPort;

    fullReq.clientHeaders = &_clientHeaders;
    fullReq.clientRequest = &_clientRequest;
    fullReq.clientContentData = &_clientContentData;

    return fullReq;
}

sHTTP_ResponseData HTTPv1_Server::responseData()
{
    sHTTP_ResponseData fullR;

    fullR.contentData = &_serverContentData;
    fullR.headers = &_serverHeaders;
    fullR.status = &_serverCodeResponse;
    fullR.setCookies = &setCookies;
    fullR.secXFrameOpts = &secXFrameOpts;
    fullR.secXSSProtection = &secXSSProtection;
    fullR.secHSTS = &secHSTS;
    fullR.bNoSniff = bNoSniff;
    fullR.contentType = contentType;

    return fullR;
}

void HTTPv1_Server::setRemotePairAddress(const char *value)
{
    strncpy(remotePairAddress,value,sizeof(remotePairAddress)-1);
}

eHTTP_ContainerType HTTPv1_Server::getRequestDataType()
{
    return _clientContentData.getContainerType();
}

Memory::Abstract::Vars *HTTPv1_Server::getRequestVars(const HTTP_VarSource &source)
{
    switch (source)
    {
    case HTTP_VARS_POST:
        return _clientContentData.postVars();
    case HTTP_VARS_GET:
        return _clientRequest.getVarsPTR();
    }
    return nullptr;
}

void HTTPv1_Server::setResponseServerName(const string &sServerName)
{
    _serverHeaders.replace("Server", sServerName);
}

bool HTTPv1_Server::setResponseFileFromURI(const string &sServerDir,string *sRealRelativePath, string *sRealFullPath)
{
    bool ret = false;
    char *cFullPath, *cServerDir;
    size_t cServerDirSize = 0;

    *sRealRelativePath="";
    *sRealFullPath="";

    // Check Server Dir Real Path:
    if ((cServerDir=realpath((sServerDir).c_str(), nullptr))==nullptr)
    {
        return false;
    }

    string sFullPath = cServerDir + getRequestURI();
    cServerDirSize = strlen(cServerDir);

    // Detect transversal:
    if ((cFullPath=realpath(sFullPath.c_str(), nullptr))!=nullptr)
    {
        if (strlen(cFullPath)<cServerDirSize || memcmp(cServerDir,cFullPath,cServerDirSize)!=0)
        {
            // Transversal directory attempt TODO: report.
        }
        else
        {
            // No transversal detected.
            CX2::Memory::Containers::B_MMAP * bFile = new CX2::Memory::Containers::B_MMAP;
            if (bFile->referenceFile(cFullPath,true,false))
            {
                // File Found / Readable.
                *sRealFullPath = sFullPath;
                *sRealRelativePath = cFullPath+cServerDirSize;
                setResponseDataStreamer(bFile,true);
                setResponseContentTypeByFileExtension(*sRealRelativePath);

                struct stat attrib;
                if (!stat(sFullPath.c_str(), &attrib))
                {
                    HTTP_Date fileModificationDate;
#ifdef WIN32
                    fileModificationDate.setRawTime(attrib.st_mtime);
#else
                    fileModificationDate.setRawTime(attrib.st_mtim.tv_sec);
#endif
                    if (includeServerDate)
                        _serverHeaders.add("Last-Modified", fileModificationDate.toString());
                }

                ret = true;
            }
            else
            {
                // File not found / Readable...
                delete bFile;
                return false;
            }
        }
    }

    if (cFullPath) free(cFullPath);

    return ret;
}

bool HTTPv1_Server::setResponseContentTypeByFileExtension(const string &sFilePath)
{
    const char * cFileExtension = strrchr(sFilePath.c_str(),'.');

    if (!cFileExtension || cFileExtension[1]==0)
        return false;

    currentFileExtension = boost::to_lower_copy(std::string(cFileExtension));

    if (mimeTypes.find(currentFileExtension) != mimeTypes.end())
    {
        setResponseContentType(mimeTypes[currentFileExtension],true);
        return true;
    }
    setResponseContentType("",false);
    return false;
}

void HTTPv1_Server::addResponseContentTypeFileExtension(const string &ext, const string &type)
{
    mimeTypes[ext] = type;
}

bool HTTPv1_Server::processClientURI()
{
    return true;
}

bool HTTPv1_Server::processClientOptions()
{
    return true;
}

eHTTP_RetCode HTTPv1_Server::processClientRequest()
{
    return HTTP_RET_200_OK;
}

bool HTTPv1_Server::changeToNextParser()
{
    // Server Mode:
    if (currentParser == &_clientRequest) return changeToNextParserOnClientRequest();
    else if (currentParser == &_clientHeaders) return changeToNextParserOnClientHeaders();
    else return changeToNextParserOnClientContentData();
}

bool HTTPv1_Server::changeToNextParserOnClientHeaders()
{
    // This function is used when the client options arrives, so we need to parse it.

    // Internal checks when options are parsed (ex. check if host exist on http/1.1)
    parseHostOptions();
    prepareServerVersionOnOptions();

    if (badAnswer)
        return answer(ansBytes);
    else
    {
        uint64_t contentLength = _clientHeaders.getOptionAsUINT64("Content-Length");
        string contentType = _clientHeaders.getOptionValueStringByName("Content-Type");
        /////////////////////////////////////////////////////////////////////////////////////
        // Content-Length...
        if (contentLength)
        {
            // Content length defined.
            _clientContentData.setTransmitionMode(HTTP_CONTENT_TRANSMODE_CONTENT_LENGTH);
            if (!_clientContentData.setContentLenSize(contentLength))
            {
                // Error setting this content length size. (automatic answer)
                badAnswer = true;
                _serverCodeResponse.setRetCode(HTTP_RET_413_PAYLOAD_TOO_LARGE);
                return answer(ansBytes);
            }
            /////////////////////////////////////////////////////////////////////////////////////
            // Content-Type... (only if length is designated)
            if ( icontains(contentType,"multipart/form-data") )
            {
                _clientContentData.setContainerType(HTTP_CONTAINERTYPE_MIME);
                _clientContentData.getMultiPartVars()->setMultiPartBoundary(_clientHeaders.getOptionByName("Content-Type")->getSubVar("boundary"));
            }
            else if ( icontains(contentType,"application/x-www-form-urlencoded") )
            {
                _clientContentData.setContainerType(HTTP_CONTAINERTYPE_URL);
            }
            else
                _clientContentData.setContainerType(HTTP_CONTAINERTYPE_BIN);
            /////////////////////////////////////////////////////////////////////////////////////
        }
        // Auth:
        // TODO: deal with http based authorization.

        // Process the client header options
        if (!badAnswer)
        {
            if (!processClientOptions())
                currentParser = nullptr; // Don't continue with parsing (close the connection)
            else
            {
                // OK, we are ready.
                if (contentLength) currentParser = &_clientContentData;
                else
                {
                    // Answer here:
                    return answer(ansBytes);
                }
            }
        }
    }
    return true;
}

bool HTTPv1_Server::changeToNextParserOnClientRequest()
{
    // Internal checks when URL request has received.
    prepareServerVersionOnURI();
    if (badAnswer)
        return answer(ansBytes);
    else
    {
        if (!processClientURI())
            currentParser = nullptr; // Don't continue with parsing.
        else currentParser = &_clientHeaders;
    }
    return true;
}

bool HTTPv1_Server::changeToNextParserOnClientContentData()
{
    return answer(ansBytes);
}

bool HTTPv1_Server::streamServerHeaders(Memory::Streams::Status &wrStat)
{
    // Act as a server. Send data from here.
    uint64_t strsize;

    if ((strsize=_serverContentData.getStreamSize()) == std::numeric_limits<uint64_t>::max())
    {
        // TODO: connection keep alive.
        _serverHeaders.add("Connetion", "Close");
        _serverHeaders.remove("Content-Length");
        /////////////////////
        if (_serverContentData.getTransmitionMode() == HTTP_CONTENT_TRANSMODE_CHUNKS)
            _serverHeaders.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        _serverHeaders.remove("Connetion");
        _serverHeaders.replace("Content-Length", std::to_string(strsize));
    }

    HTTP_Date currentDate;
    currentDate.setCurrentTime();
    if (includeServerDate)
        _serverHeaders.add("Date", currentDate.toString());

    // Establish the cookies
    _serverHeaders.remove("Set-Cookie");
    setCookies.putOnHeaders(&_serverHeaders);

    // Security Options...

    _serverHeaders.replace("X-XSS-Protection", secXSSProtection.toValue());

    if (!secXFrameOpts.isNotActivated())
        _serverHeaders.replace("X-Frame-Options", secXFrameOpts.toValue());

    // TODO: check if this is a secure connection.. (Over TLS?)
    if (secHSTS.getActivated())
        _serverHeaders.replace("Strict-Transport-Security", secHSTS.toValue());

    // Content Type...
    if (!contentType.empty())
    {
        _serverHeaders.replace("Content-Type", contentType);
        if (bNoSniff) _serverHeaders.replace("X-Content-Type-Options", "nosniff");
    }

    return _serverHeaders.stream(wrStat);
}

void HTTPv1_Server::prepareServerVersionOnURI()
{
    _serverCodeResponse.getHttpVersion()->setVersionMajor(1);
    _serverCodeResponse.getHttpVersion()->setVersionMinor(0);

    if (_clientRequest.getHTTPVersion()->getVersionMajor()!=1)
    {
        _serverCodeResponse.setRetCode(HTTP_RET_505_HTTP_VERSION_NOT_SUPPORTED);
        badAnswer = true;
    }
    else
    {
        _serverCodeResponse.getHttpVersion()->setVersionMinor(_clientRequest.getHTTPVersion()->getVersionMinor());
    }
}

void HTTPv1_Server::prepareServerVersionOnOptions()
{
    if (_clientRequest.getHTTPVersion()->getVersionMinor()>=1)
    {
        if (virtualHost=="")
        {
            // TODO: does really need the VHost?
            _serverCodeResponse.setRetCode(HTTP_RET_400_BAD_REQUEST);
            badAnswer = true;
        }
    }
}

void HTTPv1_Server::parseHostOptions()
{
    string hostVal = _clientHeaders.getOptionValueStringByName("HOST");
    if (!hostVal.empty())
    {
        virtualPort = 80;
        vector<string> hostParts;
        split(hostParts,hostVal,is_any_of(":"),token_compress_on);
        if (hostParts.size()==1)
        {
            virtualHost = hostParts[0];
        }
        else if (hostParts.size()>1)
        {
            virtualHost = hostParts[0];
            virtualPort = (uint16_t)strtoul(hostParts[1].c_str(),nullptr,10);
        }
    }
}

bool HTTPv1_Server::answer(Memory::Streams::Status &wrStat)
{
    wrStat.bytesWritten = 0;

    // Process client petition here.
    if (!badAnswer) _serverCodeResponse.setRetCode(processClientRequest());

    //  printf("@%p attending %s\n", this, _clientRequest.getURI().c_str()); fflush(stdout);

    // Answer is the last... close the connection after it.
    currentParser = nullptr;

    if (!_serverCodeResponse.stream(wrStat))
        return false;
    if (!streamServerHeaders(wrStat))
        return false;
    if (!_serverContentData.stream(wrStat))
    {
        _serverContentData.preemptiveDestroyStreamableOuput();
        return false;
    }

    // If all the data was sent OK, ret true, and destroy the external container.
    _serverContentData.preemptiveDestroyStreamableOuput();
    return true;
}

std::string HTTPv1_Server::getContentType() const
{
    return contentType;
}

std::string HTTPv1_Server::getCurrentFileExtension() const
{
    return currentFileExtension;
}

void HTTPv1_Server::setResponseIncludeServerDate(bool value)
{
    includeServerDate = value;
}

bool HTTPv1_Server::getIsSecure() const
{
    return isSecure;
}

void HTTPv1_Server::setIsSecure(bool value)
{
    isSecure = value;
}

HTTP_Security_HSTS HTTPv1_Server::getResponseSecurityHSTS() const
{
    return secHSTS;
}

void HTTPv1_Server::setResponseSecurityHSTS(const HTTP_Security_HSTS &value)
{
    secHSTS = value;
}

HTTP_Security_XSSProtection HTTPv1_Server::getResponseSecurityXSSProtection() const
{
    return secXSSProtection;
}

void HTTPv1_Server::setResponseSecurityXSSProtection(const HTTP_Security_XSSProtection &value)
{
    secXSSProtection = value;
}

HTTP_Security_XFrameOpts HTTPv1_Server::getResponseSecurityXFrameOpts() const
{
    return secXFrameOpts;
}

void HTTPv1_Server::setResponseSecurityXFrameOpts(const HTTP_Security_XFrameOpts &value)
{
    secXFrameOpts = value;
}

Memory::Streams::Status HTTPv1_Server::getResponseTransmissionStatus() const
{
    return ansBytes;
}

bool HTTPv1_Server::setResponseDeleteSecureCookie(const string &cookieName)
{
    HTTP_Cookie val;
    val.setValue("");
    val.setSecure(true);
    val.setHttpOnly(true);
    val.setToExpire();
    val.setSameSite(HTTP_COOKIE_SAMESITE_STRICT);
    return setResponseCookie(cookieName,val);
}

bool HTTPv1_Server::setResponseSecureCookie(const string &cookieName, const string &cookieValue, const uint32_t &uMaxAge)
{
    HTTP_Cookie val;
    val.setValue(cookieValue);
    val.setSecure(true);
    val.setHttpOnly(true);
    val.setExpirationInSeconds(uMaxAge);
    val.setMaxAge(uMaxAge);
    val.setSameSite(HTTP_COOKIE_SAMESITE_STRICT);
    return setResponseCookie(cookieName,val);
}

bool HTTPv1_Server::setResponseInsecureCookie(const string &sCookieName, const string &sCookieValue)
{
    HTTP_Cookie val;
    val.setValue(sCookieValue);
    return setResponseCookie(sCookieName,val);
}

bool HTTPv1_Server::setResponseCookie(const string &sCookieName, const HTTP_Cookie &sCookieValue)
{
    return setCookies.addCookieVal(sCookieName,sCookieValue);
}

Memory::Streams::Status HTTPv1_Server::streamResponse(Memory::Streams::Streamable *source)
{
    Memory::Streams::Status stat;

    if (!_serverContentData.getStreamableOuput())
    {
        stat.succeed = false;
        return stat;
    }

    source->streamTo( _serverContentData.getStreamableOuput(), stat);
    return stat;
}

Network::HTTP::eHTTP_RetCode HTTPv1_Server::setResponseRedirect(const string &location, bool temporary)
{
    _serverHeaders.replace("Location", location);
    if (temporary)
        return Network::HTTP::HTTP_RET_307_TEMPORARY_REDIRECT;
    else
        return Network::HTTP::HTTP_RET_308_PERMANENT_REDIRECT;
}

void HTTPv1_Server::setResponseContentType(const string &contentType, bool bNoSniff)
{
    this->contentType = contentType;
    this->bNoSniff = bNoSniff;
}

string HTTPv1_Server::getRequestCookie(const string &sCookieName)
{
    MIME::MIME_HeaderOption * cookiesSubVars = _clientHeaders.getOptionByName("Cookie");
    if (!cookiesSubVars) return "";
    // TODO: mayus
    return cookiesSubVars->getSubVar(sCookieName);
}

string HTTPv1_Server::getRequestContentType()
{
    return _clientHeaders.getOptionRawStringByName("Content-Type");
}

string HTTPv1_Server::getClientHeaderOption(const string &optionName)
{
    return _clientHeaders.getOptionRawStringByName(optionName);
}

uint16_t HTTPv1_Server::getRequestVirtualPort() const
{
    return virtualPort;
}

void HTTPv1_Server::setResponseDataStreamer(Memory::Streams::Streamable *dsOut, bool bDeleteAfter)
{
    _serverContentData.setStreamableOutput(dsOut,bDeleteAfter);
}

void HTTPv1_Server::setRequestDataContainer(Memory::Streams::Streamable *dsIn, bool bDeleteAfter)
{
    _clientContentData.setStreamableOutput(dsIn,bDeleteAfter);
}

Memory::Streams::Streamable *HTTPv1_Server::getResponseDataStreamer()
{
    return _serverContentData.getStreamableOuput();
}

Memory::Streams::Streamable *HTTPv1_Server::getRequestDataContainer()
{
    return _clientContentData.getStreamableOuput();
}

string HTTPv1_Server::getRequestURI()
{
    return _clientRequest.getURI();
}

string HTTPv1_Server::getRequestVirtualHost() const
{
    return virtualHost;
}
