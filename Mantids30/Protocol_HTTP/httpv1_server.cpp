#include "httpv1_server.h"

#include "hdr_cookie.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Memory/b_mmap.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <sys/stat.h>


using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

HTTP::HTTPv1_Server::HTTPv1_Server(std::shared_ptr<StreamableObject> sobject)
    : HTTPv1_Base(false, sobject)
{
    m_badAnswer = false;

    // All request will have no-cache activated.... (unless it's a real file and it's not overwritten)
    serverResponse.cacheControl.optionNoCache = true;
    serverResponse.cacheControl.optionNoStore = true;
    serverResponse.cacheControl.optionMustRevalidate = true;

    m_currentParser = (Memory::Streams::SubParser *) (&clientRequest.requestLine);

    // Default Mime Types (ref: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types)
    m_mimeTypes[".aac"] = "audio/aac";
    m_mimeTypes[".abw"] = "application/x-abiword";
    m_mimeTypes[".arc"] = "application/x-freearc";
    m_mimeTypes[".avi"] = "video/x-msvideo";
    m_mimeTypes[".azw"] = "application/vnd.amazon.ebook";
    m_mimeTypes[".bin"] = "application/octet-stream";
    m_mimeTypes[".bmp"] = "image/bmp";
    m_mimeTypes[".bz"] = "application/x-bzip";
    m_mimeTypes[".bz2"] = "application/x-bzip2";
    m_mimeTypes[".csh"] = "application/x-csh";
    m_mimeTypes[".css"] = "text/css";
    m_mimeTypes[".csv"] = "text/csv";
    m_mimeTypes[".doc"] = "application/msword";
    m_mimeTypes[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    m_mimeTypes[".eot"] = "application/vnd.ms-fontobject";
    m_mimeTypes[".epub"] = "application/epub+zip";
    m_mimeTypes[".gz"] = "application/gzip";
    m_mimeTypes[".gif"] = "image/gif";
    m_mimeTypes[".htm"] = "text/html";
    m_mimeTypes[".html"] = "text/html";
    m_mimeTypes[".iso"] = "application/octet-stream";
    m_mimeTypes[".ico"] = "image/vnd.microsoft.icon";
    m_mimeTypes[".ics"] = "text/calendar";
    m_mimeTypes[".jar"] = "application/java-archive";
    m_mimeTypes[".jpeg"] = "image/jpeg";
    m_mimeTypes[".jpg"] = "image/jpeg";
    m_mimeTypes[".js"] = "application/javascript";
    m_mimeTypes[".json"] = "application/json";
    m_mimeTypes[".jsonld"] = "application/ld+json";
    m_mimeTypes[".mid"] = "audio/midi";
    m_mimeTypes[".midi"] = "audio/x-midi";
    m_mimeTypes[".mjs"] = "text/javascript";
    m_mimeTypes[".mp3"] = "audio/mpeg";
    m_mimeTypes[".mp4"] = "video/mp4";
    m_mimeTypes[".mpeg"] = "video/mpeg";
    m_mimeTypes[".mpg"] = "video/mpeg";
    m_mimeTypes[".mpkg"] = "application/vnd.apple.installer+xml";
    m_mimeTypes[".odp"] = "application/vnd.oasis.opendocument.presentation";
    m_mimeTypes[".ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    m_mimeTypes[".odt"] = "application/vnd.oasis.opendocument.text";
    m_mimeTypes[".oga"] = "audio/ogg";
    m_mimeTypes[".ogv"] = "video/ogg";
    m_mimeTypes[".ogx"] = "application/ogg";
    m_mimeTypes[".opus"] = "audio/opus";
    m_mimeTypes[".otf"] = "font/otf";
    m_mimeTypes[".png"] = "image/png";
    m_mimeTypes[".pdf"] = "application/pdf";
    m_mimeTypes[".php"] = "application/x-httpd-php";
    m_mimeTypes[".ppt"] = "application/vnd.ms-powerpoint";
    m_mimeTypes[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    m_mimeTypes[".rar"] = "application/vnd.rar";
    m_mimeTypes[".rtf"] = "application/rtf";
    m_mimeTypes[".sh"] = "application/x-sh";
    m_mimeTypes[".svg"] = "image/svg+xml";
    m_mimeTypes[".swf"] = "application/x-shockwave-flash";
    m_mimeTypes[".tar"] = "application/x-tar";
    m_mimeTypes[".tif"] = "image/tiff";
    m_mimeTypes[".ts"] = "video/mp2t";
    m_mimeTypes[".ttf"] = "font/ttf";
    m_mimeTypes[".txt"] = "text/plain";
    m_mimeTypes[".vsd"] = "application/vnd.visio";
    m_mimeTypes[".wav"] = "audio/wav";
    m_mimeTypes[".weba"] = "audio/webm";
    m_mimeTypes[".webm"] = "video/webm";
    m_mimeTypes[".webp"] = "image/webp";
    m_mimeTypes[".woff"] = "font/woff";
    m_mimeTypes[".woff2"] = "font/woff2";
    m_mimeTypes[".xhtml"] = "application/xhtml+xml";
    m_mimeTypes[".xls"] = "application/vnd.ms-excel";
    m_mimeTypes[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    m_mimeTypes[".xml"] = "application/xml";
    m_mimeTypes[".xul"] = "application/vnd.mozilla.xul+xml";
    m_mimeTypes[".zip"] = "application/zip";
    m_mimeTypes[".3gp"] = "video/3gpp";
    m_mimeTypes[".3g2"] = "video/3gpp2";
    m_mimeTypes[".7z"] = "application/x-7z-compressed";

    m_includeServerDate = true;
}

void HTTP::HTTPv1_Server::setResponseServerName(const string &sServerName)
{
    serverResponse.headers.replace("Server", sServerName);
}

bool HTTP::HTTPv1_Server::getLocalFilePathFromURI2(string defaultWebRootWithEndingSlash, const std::list<std::pair<std::string, std::string>> &overlappedDirectories, sLocalRequestedFileInfo *info,
                                                   const std::string &defaultFileToAppend, const bool &preventMappingExecutables)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    std::string requestedURI = clientRequest.getURI();

    auto resolveDirPathWithSlashAtEnd = [](const std::string &serverDirectoryPath) -> std::string
    {
        // Use unique_ptr for automatic memory management
        std::unique_ptr<char, decltype(&free)> resolvedPathUniquePtr(realpath(serverDirectoryPath.c_str(), nullptr), &free);
        if (!resolvedPathUniquePtr)
        {
            return "";
        }
        std::string resolvedPath(resolvedPathUniquePtr.get());
        // Put a slash at the end of the server dir resource...
        if (!resolvedPath.empty() && resolvedPath.back() != SLASHB)
        {
            resolvedPath += SLASH;
        }
        return resolvedPath;
    };

    defaultWebRootWithEndingSlash = resolveDirPathWithSlashAtEnd(defaultWebRootWithEndingSlash);
    if (defaultWebRootWithEndingSlash.empty())
    {
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create list of potential paths to check

    struct RequestedOverlapInfo
    {
        bool detectPathTraversal()
        {
            if (fileSystemRealPath.size() < serverWebRootWithEndingSlash.size()                                                    // outside dir?
                || memcmp(serverWebRootWithEndingSlash.c_str(), fileSystemRealPath.c_str(), serverWebRootWithEndingSlash.size()) != 0 // not matching?
            )
            {
                return true;
            }
            return false;
        }

        std::string getRelativePath()
        {
            // Eg. Relative Path: /assets/style.css
            return urlPathPrefix + fileSystemRealPath.substr(serverWebRootWithEndingSlash.size());
        }

        std::string fileSystemRealPath; // Eg. /var/assets/style.css or /var/assets/js/ for dir
        std::string serverWebRootWithEndingSlash; // Eg. /var/assets/
        std::string urlPathPrefix; // Eg. /assets/, /
        struct stat fileStats;
    };

    std::vector<RequestedOverlapInfo> detectedPotentialOverlaps;
    std::string requestURIWithoutLeadingSlash = (requestedURI.size()<=1) ? "/" : requestedURI.substr(1);
    std::string requestURIWithDefaultFile = requestURIWithoutLeadingSlash + defaultFileToAppend;
    detectedPotentialOverlaps.push_back({defaultWebRootWithEndingSlash + requestURIWithDefaultFile, defaultWebRootWithEndingSlash, "/"}); // Add the original path

    // Add overlapped directories if they match the requested URI
    for (const auto &overlappedDirectory : overlappedDirectories)
    {
        // All the overlapped dirs should end with /
        if (overlappedDirectory.first.empty() || overlappedDirectory.second.empty())
            continue;

        if (boost::starts_with(requestedURI, overlappedDirectory.first))
        {
            std::string overlappedWebRootWithEndingSlash = resolveDirPathWithSlashAtEnd(overlappedDirectory.second);
            if (overlappedWebRootWithEndingSlash.empty())
            {
                continue;
            }

            // Compute the full path with overlapped directory
            RequestedOverlapInfo oInfo = {overlappedWebRootWithEndingSlash + requestURIWithDefaultFile.substr(overlappedDirectory.first.size() - 1), overlappedWebRootWithEndingSlash, overlappedDirectory.first};
            detectedPotentialOverlaps.push_back(oInfo);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Compute the full requested path:
    RequestedOverlapInfo selectedOverlap;
    {
        char *cFullPath = nullptr;
        if (m_staticContentElements.find(requestURIWithDefaultFile) != m_staticContentElements.end())
        {
            // STATIC CONTENT:
            serverResponse.cacheControl.optionNoCache = false;
            serverResponse.cacheControl.optionNoStore = false;
            serverResponse.cacheControl.optionMustRevalidate = false;
            serverResponse.cacheControl.maxAge = 3600;
            serverResponse.cacheControl.optionImmutable = true;

            info->sRealRelativePath = requestedURI + defaultFileToAppend;

            setResponseContentTypeByFileExtension(info->sRealRelativePath);

            info->sRealFullPath = "MEM:" + info->sRealRelativePath;

            serverResponse.setDataStreamer(m_staticContentElements[info->sRealRelativePath]);
            return true;
        }
        else
        {
            bool pathFound = false;

            for (const auto &rpInfo : detectedPotentialOverlaps)
            {
                selectedOverlap = rpInfo;
                if ((cFullPath = realpath(rpInfo.fileSystemRealPath.c_str(), nullptr)) != nullptr)
                {
                    // Compute the full path..
                    selectedOverlap.fileSystemRealPath = cFullPath;
                    free(cFullPath);

                    // Check file properties...
                    stat(selectedOverlap.fileSystemRealPath.c_str(), &selectedOverlap.fileStats);

                    // Put a slash at the end of the computed dir resource (when dir)...
                    if ((info->isDir = S_ISDIR(selectedOverlap.fileStats.st_mode)) == true)
                        selectedOverlap.fileSystemRealPath += (selectedOverlap.fileSystemRealPath.back() == SLASHB ? "" : std::string(SLASH));

                    // Path OK, continue.

                    // Check for transversal access hacking attempts...
                    if (selectedOverlap.detectPathTraversal())
                    {
                        info->isTransversal = true;
                        return false;
                    }

                    pathFound = true;
                    break;
                }
            }

            if (!pathFound)
                return false;
        }
    }

    // No transversal detected at this point.

    // Check if it's a directory...

    if (info->isDir == true)
    {
        info->pathExist = true;

        // Don't get directories when we are appending something.
        if (!defaultFileToAppend.empty())
            return false;

        info->sRealFullPath = selectedOverlap.fileSystemRealPath;
        info->sRealRelativePath = selectedOverlap.getRelativePath();

        // Do we have access?:
        return !access(info->sRealFullPath.c_str(), R_OK);
    }
    else if (S_ISREG(selectedOverlap.fileStats.st_mode) == true) // Check if it's a regular file
    {
        info->pathExist = true;

        if (preventMappingExecutables &&
#ifndef _WIN32
            !access(selectedOverlap.fileSystemRealPath.c_str(), X_OK)
#else
            (boost::iends_with(requestedPathInfo.fsPath, ".exe") || boost::iends_with(requestedPathInfo.fsPath, ".bat") || boost::iends_with(requestedPathInfo.fsPath, ".com"))
#endif
        )
        {
            // file is executable... don't map, and the most important: don't create cache in the browser...
            // Very useful for CGI-like implementations...
            info->sRealFullPath = selectedOverlap.fileSystemRealPath;
            info->sRealRelativePath = selectedOverlap.getRelativePath();
            info->isExecutable = true;
            info->pathExist = true;
            return true;
        }
        else
        {
            std::shared_ptr<Mantids30::Memory::Containers::B_MMAP> fileMemoryMap = std::make_shared<Mantids30::Memory::Containers::B_MMAP>();
            if (fileMemoryMap->referenceFile(selectedOverlap.fileSystemRealPath.c_str(), true, false))
            {
                // File Found / Readable.
                info->sRealFullPath = selectedOverlap.fileSystemRealPath;
                info->sRealRelativePath = selectedOverlap.getRelativePath();
                serverResponse.setDataStreamer(fileMemoryMap);
                setResponseContentTypeByFileExtension(info->sRealRelativePath);

                HTTP::Date fileModificationDate;
#ifdef _WIN32
                fileModificationDate.setUnixTime(selectedPathInfo.fileStats.st_mtime);
#else
                fileModificationDate.setUnixTime(selectedOverlap.fileStats.st_mtim.tv_sec);
#endif
                if (m_includeServerDate)
                    serverResponse.headers.add("Last-Modified", fileModificationDate.toString());

                serverResponse.cacheControl.optionNoCache = false;
                serverResponse.cacheControl.optionNoStore = false;
                serverResponse.cacheControl.optionMustRevalidate = false;
                serverResponse.cacheControl.maxAge = 3600;
                serverResponse.cacheControl.optionImmutable = true;
                return true;
            }
            return false;
        }
    }
    else
    {
        // Special files...
        return false;
    }
}

bool HTTP::HTTPv1_Server::getLocalFilePathFromURI0NE(const std::string &uri, std::string sServerDir, sLocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath = sServerDir                           // Put the current server dir...
                                + (uri.empty() ? "" : uri.substr(1)) // Put the Request URI (without the first character / slash)
        ;                                                            // Append option...

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX];
        if (realpath(sFullRequestedPath.c_str(), cRealPath) == nullptr)
        {
            if (errno == ENOENT)
            {
                // Non-existent file.
                return false; // or handle the error as needed
            }
            else
            {
                // Other error occurred.
                return false; // or handle the error as needed
            }
        }
        else
        {
            sFullComputedRealPath = cRealPath;
        }
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size() < sServerDir.size() || memcmp(sServerDir.c_str(), sFullComputedRealPath.c_str(), sServerDir.size()) != 0)
    {
        info->isTransversal = true;
        return false;
    }

    // No transversal detected at this point.
    info->sRealFullPath = sFullComputedRealPath;
    info->sRealRelativePath = sFullComputedRealPath.c_str() + (sServerDir.size() - 1);

    return true;
}

bool HTTP::HTTPv1_Server::getLocalFilePathFromURI0E(const std::string &uri, std::string sServerDir, sLocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error(std::string(__func__) + std::string(" Should be called with info object... Aborting..."));

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir = realpath((sServerDir).c_str(), nullptr)) == nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath = sServerDir                           // Put the current server dir...
                                + (uri.empty() ? "" : uri.substr(1)) // Put the Request URI (without the first character / slash)
        ;                                                            // Append option...

    struct stat stats;

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX + 2];
        if (realpath(sFullRequestedPath.c_str(), cRealPath))
        {
            sFullComputedRealPath = cRealPath;

            // Check file properties...
            stat(sFullComputedRealPath.c_str(), &stats);
            // Put a slash at the end of the computed dir resource (when dir)...
            if ((info->isDir = S_ISDIR(stats.st_mode)) == true)
                sFullComputedRealPath += (sFullComputedRealPath.back() == SLASHB ? "" : std::string(SLASH));
        }
        else // Non-Existant File.
            return false;
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size() < sServerDir.size() || memcmp(sServerDir.c_str(), sFullComputedRealPath.c_str(), sServerDir.size()) != 0)
    {
        info->isTransversal = true;
        return false;
    }

    // No transversal detected at this point.
    info->sRealFullPath = sFullComputedRealPath;
    info->sRealRelativePath = sFullComputedRealPath.c_str() + (sServerDir.size() - 1);

    return true;
}

bool HTTP::HTTPv1_Server::setResponseContentTypeByFileExtension(const string &sFilePath)
{
    const char *cFileExtension = strrchr(sFilePath.c_str(), '.');

    if (!cFileExtension || cFileExtension[1] == 0)
        return false;

    m_currentFileExtension = boost::to_lower_copy(std::string(cFileExtension));

    if (m_mimeTypes.find(m_currentFileExtension) != m_mimeTypes.end())
    {
        serverResponse.setContentType(m_mimeTypes[m_currentFileExtension], true);
        return true;
    }
    serverResponse.setContentType("", false);
    return false;
}

void HTTP::HTTPv1_Server::addResponseContentTypeFileExtension(const string &ext, const string &type)
{
    m_mimeTypes[ext] = type;
}

bool HTTP::HTTPv1_Server::changeToNextParser()
{
    // Server Mode:
    if (m_currentParser == &clientRequest.requestLine)
        return changeToNextParserFromClientRequestLine();
    else if (m_currentParser == &clientRequest.headers)
        return changeToNextParserFromClientHeaders();
    else
        return changeToNextParserFromClientContentData();
}

void HTTP::HTTPv1_Server::setClientInfoVars(const char *ipAddr, const bool &secure, const std::string &tlsCommonName)
{
    clientRequest.networkClientInfo.tlsCommonName = tlsCommonName;
    strncpy(clientRequest.networkClientInfo.REMOTE_ADDR, ipAddr, sizeof(clientRequest.networkClientInfo.REMOTE_ADDR));
    clientRequest.networkClientInfo.isSecure = secure;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientHeaders()
{
    // This function is used when the client options arrives, so we need to parse it.

    // Internal checks when options are parsed (ex. check if host exist on http/1.1)
    parseHostOptions();

    prepareServerVersionOnOptions();

    // PARSE CLIENT BASIC AUTHENTICATION:
    clientRequest.basicAuth.isEnabled = false;
    if (clientRequest.headers.exist("Authorization"))
    {
        vector<string> authParts;
        string f1 = clientRequest.headers.getOptionValueStringByName("Authorization");
        split(authParts, f1, is_any_of(" "), token_compress_on);
        if (authParts.size() == 2)
        {
            if (authParts[0] == "Basic")
            {
                auto bp = Helpers::Encoders::decodeFromBase64(authParts[1]);
                std::string::size_type pos = bp.find(':', 0);
                if (pos != std::string::npos)
                {
                    clientRequest.basicAuth.isEnabled = true;
                    clientRequest.basicAuth.username = bp.substr(0, pos);
                    clientRequest.basicAuth.password = bp.substr(pos + 1, bp.size());
                }
            }
        }
    }

    // PARSE USER-AGENT
    if (clientRequest.headers.exist("User-Agent"))
        clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");

    // PARSE CONTENT TYPE/LENGHT OPTIONS
    if (m_badAnswer)
        return answer();
    else
    {
        size_t contentLength = clientRequest.headers.getOptionAsUINT64("Content-Length");
        string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
        /////////////////////////////////////////////////////////////////////////////////////
        // Content-Length...
        if (contentLength)
        {
            // Content length defined.
            clientRequest.content.setTransmitionMode(HTTP::Content::TRANSMIT_MODE_CONTENT_LENGTH);
            if (!clientRequest.content.setContentLenSize(contentLength))
            {
                // Error setting this content length size. (automatic answer)
                m_badAnswer = true;
                serverResponse.status.setCode(HTTP::Status::S_413_PAYLOAD_TOO_LARGE);
                return answer();
            }
            /////////////////////////////////////////////////////////////////////////////////////
            // Content-Type... (only if length is designated)
            if (icontains(contentType, "multipart/form-data"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_MIME);
                clientRequest.content.getMultiPartVars()->setMultiPartBoundary(clientRequest.headers.getOptionByName("Content-Type")->getSubVar("boundary"));
            }
            else if (icontains(contentType, "application/x-www-form-urlencoded"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_URL);
            }
            else if (icontains(contentType, "application/json"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_JSON);
            }
            else
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_BIN);
            /////////////////////////////////////////////////////////////////////////////////////
        }

        // Process the client header options
        if (!m_badAnswer)
        {
            //setClientInfoVars();

            if (!procHTTPClientHeaders())
                m_currentParser = nullptr; // Don't continue with parsing (close the connection)
            else
            {
                // OK, we are ready.
                if (contentLength)
                    m_currentParser = &clientRequest.content;
                else
                {
                    // Answer here:
                    return answer();
                }
            }
        }
    }
    return true;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientRequestLine()
{
    // Internal checks when URL request has received.
    prepareServerVersionOnURI();
    if (m_badAnswer)
        return answer();
    else
    {
        //setClientInfoVars();
        if (!procHTTPClientURI())
            m_currentParser = nullptr; // Don't continue with parsing.
        else
            m_currentParser = &clientRequest.headers;
    }
    return true;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientContentData()
{
    m_currentParser = nullptr; // Don't continue with parsing. (TODO: Not supported yet)
    return answer();
}

bool HTTP::HTTPv1_Server::streamServerHeaders()
{
    // Act as a server. Send data from here.
    size_t strsize;

    if ((strsize = serverResponse.content.getStreamSize()) == std::numeric_limits<size_t>::max())
    {
        // TODO: connection keep alive.
        serverResponse.headers.replace("Connetion", "Close");
        serverResponse.headers.remove("Content-Length");
        /////////////////////
        if (serverResponse.content.getTransmitionMode() == HTTP::Content::TRANSMIT_MODE_CHUNKS)
            serverResponse.headers.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        serverResponse.headers.remove("Connetion");
        serverResponse.headers.replace("Content-Length", std::to_string(strsize));
    }

    HTTP::Date currentDate;
    currentDate.setCurrentTime();
    if (m_includeServerDate)
        serverResponse.headers.replace("Date", currentDate.toString());

    if (serverResponse.immutableHeaders)
    {
        // No futher headers will be modified...
        return serverResponse.headers.streamToUpstream();
    }

    if (!serverResponse.sWWWAuthenticateRealm.empty())
    {
        serverResponse.headers.replace("WWW-Authenticate", "Basic realm=\"" + serverResponse.sWWWAuthenticateRealm + "\"");
    }

    // Establish the cookies
    serverResponse.headers.remove("Set-Cookie");
    serverResponse.cookies.putOnHeaders(&serverResponse.headers);

    // Security Options...
    serverResponse.headers.replace("X-XSS-Protection", serverResponse.security.XSSProtection.toString());

    std::string cacheOptions = serverResponse.cacheControl.toString();
    if (!cacheOptions.empty())
        serverResponse.headers.replace("Cache-Control", cacheOptions);

    if (!serverResponse.security.XFrameOpts.isNotActivated())
        serverResponse.headers.replace("X-Frame-Options", serverResponse.security.XFrameOpts.toString());

    // TODO: check if this is a secure connection.. (Over TLS?)
    if (serverResponse.security.HSTS.isActivated)
        serverResponse.headers.replace("Strict-Transport-Security", serverResponse.security.HSTS.toString());

    // Content Type...
    if (!serverResponse.contentType.empty())
    {
        serverResponse.headers.replace("Content-Type", serverResponse.contentType);
        if (serverResponse.security.disableNoSniffContentType)
            serverResponse.headers.replace("X-Content-Type-Options", "nosniff");
    }

    return serverResponse.headers.streamToUpstream();
}

void HTTP::HTTPv1_Server::prepareServerVersionOnURI()
{
    serverResponse.status.getHTTPVersion()->setMajor(1);
    serverResponse.status.getHTTPVersion()->setMinor(0);

    if (clientRequest.requestLine.getHTTPVersion()->getMajor() != 1)
    {
        serverResponse.status.setCode(HTTP::Status::S_505_HTTP_VERSION_NOT_SUPPORTED);
        m_badAnswer = true;
    }
    else
    {
        serverResponse.status.getHTTPVersion()->setMinor(clientRequest.requestLine.getHTTPVersion()->getMinor());
    }
}

void HTTP::HTTPv1_Server::prepareServerVersionOnOptions()
{
    if (clientRequest.requestLine.getHTTPVersion()->getMinor() >= 1)
    {
        if (clientRequest.virtualHost == "")
        {
            serverResponse.status.setCode(HTTP::Status::S_400_BAD_REQUEST);
            m_badAnswer = true;
        }
    }
}

void HTTP::HTTPv1_Server::parseHostOptions()
{
    string hostVal = clientRequest.headers.getOptionValueStringByName("HOST");
    if (!hostVal.empty())
    {
        clientRequest.virtualPort = 80;
        vector<string> hostParts;
        split(hostParts, hostVal, is_any_of(":"), token_compress_on);
        if (hostParts.size() == 1)
        {
            clientRequest.virtualHost = hostParts[0];
        }
        else if (hostParts.size() > 1)
        {
            clientRequest.virtualHost = hostParts[0];
            clientRequest.virtualPort = (uint16_t) strtoul(hostParts[1].c_str(), nullptr, 10);
        }
    }
}

bool HTTP::HTTPv1_Server::answer()
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "HTTP:Response");
#endif

    // Process client petition here.
    if (!m_badAnswer)
    {
        serverResponse.status.setCode(procHTTPClientContent());
    }

    // Answer is the last... close the connection after it.
    m_currentParser = nullptr;

    if (!serverResponse.status.streamToUpstream())
    {
        return false;
    }

    if (!streamServerHeaders())
    {
        return false;
    }

    bool streamedOK = serverResponse.content.streamToUpstream();

    // Destroy the binary content container here:
    serverResponse.content.setStreamableObj(nullptr);

    return streamedOK;
}

void HTTP::HTTPv1_Server::setStaticContentElements(const std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM>> &value)
{
    m_staticContentElements = value;
}

std::string HTTP::HTTPv1_Server::htmlEncode(const std::string &rawStr)
{
    std::string output;
    output.reserve(rawStr.size());
    for (size_t i = 0; rawStr.size() != i; i++)
    {
        switch (rawStr[i])
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

bool HTTP::HTTPv1_Server::verifyStaticContentExistence(const string &path)
{
    return !(m_staticContentElements.find(path) == m_staticContentElements.end());
}

std::string HTTP::HTTPv1_Server::getCurrentFileExtension() const
{
    return m_currentFileExtension;
}

void HTTP::HTTPv1_Server::setResponseIncludeServerDate(bool value)
{
    m_includeServerDate = value;
}

void HTTP::HTTPv1_Server::addStaticContent(const string &path, std::shared_ptr<Memory::Containers::B_MEM> contentElement)
{
    m_staticContentElements[path] = contentElement;
}

bool HTTP::HTTPv1_Server::streamResponse(std::shared_ptr<Memory::Streams::StreamableObject> source)
{
    if (!serverResponse.content.getStreamableObj())
    {
        return false;
    }
    // Stream in place:
    source->streamTo(serverResponse.content.getStreamableObj().get());
    return true;
}

std::shared_ptr<Memory::Streams::StreamableObject> HTTP::HTTPv1_Server::getResponseDataStreamer()
{
    return serverResponse.content.getStreamableObj();
}
