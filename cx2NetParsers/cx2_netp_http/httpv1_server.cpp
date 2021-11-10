#include "httpv1_server.h"

#include "hdr_cookie.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <cx2_hlp_functions/encoders.h>
#include <cx2_mem_vars/b_mmap.h>

#include <sys/stat.h>

#ifdef _WIN32
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

    // All request will have no-cache activated.... (unless it's a real file and it's not overwritten)
    cacheControl.setOptionNoCache(true);
    cacheControl.setOptionNoStore(true);
    cacheControl.setOptionMustRevalidate(true);

    remotePairAddress[0]=0;
    currentParser = (Memory::Streams::Parsing::SubParser *)(&_clientRequestLine);

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

Request::DataObjects HTTPv1_Server::getRequestActiveObjects()
{
    Request::DataObjects fullReq;

    fullReq.USING_BASIC_AUTH = false;

    if (_clientHeaders.exist("Authorization"))
    {
        vector<string> authParts;
        string f1 = _clientHeaders.getOptionValueStringByName("Authorization");
        split(authParts,f1,is_any_of(" "),token_compress_on);
        if (authParts.size()==2)
        {
            if (authParts[0] == "Basic")
            {
                auto bp = Helpers::Encoders::fromBase64(authParts[1]);

                std::string::size_type pos = bp.find(':', 0);

                if (pos != std::string::npos)
                {
                    fullReq.USING_BASIC_AUTH = true;
                    fullReq.AUTH_USER = bp.substr(0,pos);
                    fullReq.AUTH_PASS = bp.substr(pos+1,bp.size());
                }

            }
        }
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
    fullReq.clientRequestLine = &_clientRequestLine;
    fullReq.clientContentData = &_clientContentData;

    return fullReq;
}

Response::DataObject HTTPv1_Server::getResponseActiveObjects()
{
    Response::DataObject fullR;

    fullR.contentData = &_serverContentData;
    fullR.headers = &_serverHeaders;
    fullR.status = &_serverCodeResponse;
    fullR.setCookies = &setCookies;
    fullR.secXFrameOpts = &secXFrameOpts;
    fullR.secXSSProtection = &secXSSProtection;
    fullR.secHSTS = &secHSTS;
    fullR.bNoSniff = bNoSniff;
    fullR.contentType = &contentType;
    fullR.authenticate = &authenticate;
    fullR.cacheControl = &cacheControl;

    return fullR;
}

void HTTPv1_Server::setRemotePairAddress(const char *value)
{
    SecBACopy(remotePairAddress,value);
}

Common::eContent_DataType HTTPv1_Server::getRequestDataType()
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
        return _clientRequestLine.getVarsPTR();
    }
    return nullptr;
}

void HTTPv1_Server::setResponseServerName(const string &sServerName)
{
    _serverHeaders.replace("Server", sServerName);
}

bool HTTPv1_Server::getLocalFilePathFromURI(const string &sServerDir, string *sRealRelativePath, string *sRealFullPath, const string &defaultFileAppend, bool *isDir)
{
    bool ret = false;
    char *cFullPath, *cServerDir;
    size_t cServerDirSize = 0;

    *sRealRelativePath="";
    *sRealFullPath="";

    if (isDir)
        *isDir = false;

    // Check Server Dir Real Path:
    if ((cServerDir=realpath((sServerDir).c_str(), nullptr))==nullptr)
    {
        return false;
    }

    string sFullPath = cServerDir + getRequestURI() + defaultFileAppend;
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

            // looking for dir...
            if (getRequestURI().back() == '/' && defaultFileAppend.empty())
            {
                // Maybe is a directory...
                if (isDir)
                {
                    struct stat stats;

                    stat(cFullPath, &stats);

                    // Check if it's a directory...
                    *isDir = S_ISDIR(stats.st_mode);

                    if (*isDir)
                    {
                        *sRealFullPath = sFullPath;
                        *sRealRelativePath = cFullPath+cServerDirSize;

                        ret = true;
                    }
                }
            }
            // looking for file...
            else
            {
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
                        HTTP::Common::Date fileModificationDate;
#ifdef _WIN32
                        fileModificationDate.setRawTime(attrib.st_mtime);
#else
                        fileModificationDate.setRawTime(attrib.st_mtim.tv_sec);
#endif
                        if (includeServerDate)
                            _serverHeaders.add("Last-Modified", fileModificationDate.toString());
                    }

                    cacheControl.setOptionNoCache(false);
                    cacheControl.setOptionNoStore(false);
                    cacheControl.setOptionMustRevalidate(false);
                    cacheControl.setMaxAge(3600);
                    cacheControl.setOptionImmutable(true);
                    ret = true;
                }
                else
                {
                    // File not found / Readable...
                    delete bFile;
                    ret = false;
                }
            }
        }
    }
    else if (  staticContentElements.find(std::string(getRequestURI()+defaultFileAppend)) != staticContentElements.end() )
    { // STATIC CONTENT:
        *sRealRelativePath = getRequestURI()+defaultFileAppend;
        *sRealFullPath = "MEM:" + *sRealRelativePath;
        setResponseDataStreamer(staticContentElements[*sRealRelativePath],false);
        ret = true;
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

Response::StatusCode HTTPv1_Server::processClientRequest()
{
    return Response::StatusCode::S_200_OK;
}

bool HTTPv1_Server::changeToNextParser()
{
    // Server Mode:
    if (currentParser == &_clientRequestLine) return changeToNextParserOnClientRequest();
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
            _clientContentData.setTransmitionMode(Common::TRANSMIT_MODE_CONTENT_LENGTH);
            if (!_clientContentData.setContentLenSize(contentLength))
            {
                // Error setting this content length size. (automatic answer)
                badAnswer = true;
                _serverCodeResponse.setRetCode(Response::StatusCode::S_413_PAYLOAD_TOO_LARGE);
                return answer(ansBytes);
            }
            /////////////////////////////////////////////////////////////////////////////////////
            // Content-Type... (only if length is designated)
            if ( icontains(contentType,"multipart/form-data") )
            {
                _clientContentData.setContainerType(Common::CONTENT_TYPE_MIME);
                _clientContentData.getMultiPartVars()->setMultiPartBoundary(_clientHeaders.getOptionByName("Content-Type")->getSubVar("boundary"));
            }
            else if ( icontains(contentType,"application/x-www-form-urlencoded") )
            {
                _clientContentData.setContainerType(Common::CONTENT_TYPE_URL);
            }
            else
                _clientContentData.setContainerType(Common::CONTENT_TYPE_BIN);
            /////////////////////////////////////////////////////////////////////////////////////
        }

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
        if (_serverContentData.getTransmitionMode() == Common::TRANSMIT_MODE_CHUNKS)
            _serverHeaders.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        _serverHeaders.remove("Connetion");
        _serverHeaders.replace("Content-Length", std::to_string(strsize));
    }

    HTTP::Common::Date currentDate;
    currentDate.setCurrentTime();
    if (includeServerDate)
        _serverHeaders.add("Date", currentDate.toString());

    if (!authenticate.empty())
    {
        _serverHeaders.add("WWW-Authenticate", "Basic realm=\""+ authenticate + "\"");
    }

    // Establish the cookies
    _serverHeaders.remove("Set-Cookie");
    setCookies.putOnHeaders(&_serverHeaders);

    // Security Options...

    _serverHeaders.replace("X-XSS-Protection", secXSSProtection.toValue());

    std::string cacheOptions = cacheControl.toString();
    if (!cacheOptions.empty())
        _serverHeaders.replace("Cache-Control", cacheOptions);

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

    if (_clientRequestLine.getHTTPVersion()->getVersionMajor()!=1)
    {
        _serverCodeResponse.setRetCode(Response::StatusCode::S_505_HTTP_VERSION_NOT_SUPPORTED);
        badAnswer = true;
    }
    else
    {
        _serverCodeResponse.getHttpVersion()->setVersionMinor(_clientRequestLine.getHTTPVersion()->getVersionMinor());
    }
}

void HTTPv1_Server::prepareServerVersionOnOptions()
{
    if (_clientRequestLine.getHTTPVersion()->getVersionMinor()>=1)
    {
        if (virtualHost=="")
        {
            // TODO: does really need the VHost?
            _serverCodeResponse.setRetCode(Response::StatusCode::S_400_BAD_REQUEST);
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

void HTTPv1_Server::setStaticContentElements(const std::map<std::string, CX2::Memory::Containers::B_MEM *> &value)
{
    staticContentElements = value;
}

std::string HTTPv1_Server::htmlEncode(const std::string &rawStr)
{
    std::string output;
    output.reserve(rawStr.size());
    for(size_t i=0; rawStr.size()!=i; i++)
    {
        switch(rawStr[i])
        {
        case '<':
            output.append("&lt;");
            break;
        case '>':
            output.append("&gt;");
            break;
        case '\"':
            output.append("&quot;");
            break;
        case '&':
            output.append("&amp;");
            break;
        case '\'':
            output.append("&apos;");
            break;
        default:
            output.append(&rawStr[i], 1);
            break;
        }
    }
    return output;
}

bool HTTPv1_Server::verifyStaticContentExistence(const string &path)
{
    return !(staticContentElements.find(path) == staticContentElements.end());
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

void HTTPv1_Server::addStaticContent(const string &path, Memory::Containers::B_MEM *contentElement)
{
    staticContentElements[path] = contentElement;
}

Headers::Security::HSTS HTTPv1_Server::getResponseSecurityHSTS() const
{
    return secHSTS;
}

void HTTPv1_Server::setResponseSecurityHSTS(const Headers::Security::HSTS &value)
{
    secHSTS = value;
}

Headers::Security::XSSProtection HTTPv1_Server::getResponseSecurityXSSProtection() const
{
    return secXSSProtection;
}

void HTTPv1_Server::setResponseSecurityXSSProtection(const Headers::Security::XSSProtection &value)
{
    secXSSProtection = value;
}

Headers::Security::XFrameOpts HTTPv1_Server::getResponseSecurityXFrameOpts() const
{
    return secXFrameOpts;
}

void HTTPv1_Server::setResponseSecurityXFrameOpts(const Headers::Security::XFrameOpts &value)
{
    secXFrameOpts = value;
}

Memory::Streams::Status HTTPv1_Server::getResponseTransmissionStatus() const
{
    return ansBytes;
}

void HTTPv1_Server::addCookieClearSecure(const string &cookieName)
{
    setCookies.addClearSecureCookie(cookieName);
}

bool HTTPv1_Server::setResponseSecureCookie(const string &cookieName, const string &cookieValue, const uint32_t &uMaxAge)
{
    Headers::Cookie val;
    val.setValue(cookieValue);
    val.setSecure(true);
    val.setHttpOnly(true);
    val.setExpirationInSeconds(uMaxAge);
    val.setMaxAge(uMaxAge);
    val.setSameSite(HTTP::Headers::HTTP_COOKIE_SAMESITE_STRICT);
    return setResponseCookie(cookieName,val);
}

bool HTTPv1_Server::setResponseInsecureCookie(const string &sCookieName, const string &sCookieValue)
{
    Headers::Cookie val;
    val.setValue(sCookieValue);
    return setResponseCookie(sCookieName,val);
}

bool HTTPv1_Server::setResponseCookie(const string &sCookieName, const Headers::Cookie &sCookieValue)
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

Network::HTTP::Response::StatusCode HTTPv1_Server::setResponseRedirect(const string &location, bool temporary)
{
    _serverHeaders.replace("Location", location);
    if (temporary)
        return Network::HTTP::Response::StatusCode::S_307_TEMPORARY_REDIRECT;
    else
        return Network::HTTP::Response::StatusCode::S_308_PERMANENT_REDIRECT;
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
    return _clientRequestLine.getURI();
}

string HTTPv1_Server::getRequestVirtualHost() const
{
    return virtualHost;
}
