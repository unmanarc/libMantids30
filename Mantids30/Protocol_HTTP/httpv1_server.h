#pragma once

#include "httpv1_base.h"
#include <memory>

#ifdef _WIN32
#define SLASHB '\\'
#define SLASH "\\"
#include <boost/algorithm/string/predicate.hpp>
#include <stdlib.h>
#define realpath(N, R) _fullpath((R), (N), _MAX_PATH)
#else
#define SLASH "/"
#define SLASHB '/'
#endif

// TODO: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Access-Control-Allow-Credentials

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

namespace Mantids30::Network::Protocols::HTTP {


class HTTPv1_Server : public HTTPv1_Base
{
public:
    struct LocalRequestedFileInfo
    {
        // Default constructor (implicitly defaulted)
        LocalRequestedFileInfo() = default;
        void reset()
        {
            // Reassign using aggregate initialization
            *this = LocalRequestedFileInfo{};
        }

        std::string relativePath;
        std::string fullPath;
        bool isDirectory = false;
        bool isExecutable = false;
        bool isTransversal = false;
        bool exists = false;
    };

    HTTPv1_Server(std::shared_ptr<Memory::Streams::StreamableObject> connectionStream);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // RESPONSE:
    /**
     * @brief addFileExtensionMimeType Add/Replace File Extension to Mime Content Type Association (no Thread-Safe, must be done before start the server)
     * @param ext extension (important!!: should be in lowercase)
     * @param content type
     */
    void setMimeTypeForFileExtension(const std::string & ext, const std::string & type);

    /**
     * @brief getResponseDataStreamer Get the response data streamer
     * @return
     */
    std::shared_ptr<Memory::Streams::StreamableObject> getResponseDataStreamer();

    /**
     * @brief streamResponse Stream Response to data streamer container (may copy bytes into a container, don't use for massive data transfers)
     * @return Status of the Operation
     */
    bool streamResponse(std::shared_ptr<Memory::Streams::StreamableObject> source);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OTHER FUNCTIONS:
    /**
     * @brief addStaticContent Adds a static content element mapped to a specific path.
     * This function allows adding a memory-based content element (e.g., a file or resource) to the server's static content map.
     * The content is associated with a specific path, which will be used to serve it when requested.
     * @param path The path under which the content will be served.
     * @param contentElement The shared pointer to the memory content element to be served.
     */
    void addStaticContent(const std::string & path, std::shared_ptr<Mantids30::Memory::Containers::B_MEM> contentElement);

    /**
     * @brief setStaticContentElements Sets the entire map of static content elements.
     * This function replaces the current static content map with the provided one.
     * It's useful for bulk updates or initialization of static content.
     * @param value A map of paths to memory content elements.
     */
    void setStaticContentElements(const std::map<std::string, std::shared_ptr<Memory::Containers::B_MEM> > &value);

    /**
     * @brief htmlEncode Encodes a raw string into HTML-safe format.
     * This function escapes special HTML characters (like <, >, &, ", ') to prevent XSS vulnerabilities.
     * @param rawStr The input string to be HTML-encoded.
     * @return The HTML-encoded version of the input string.
     */
    static std::string htmlEncode(const std::string& rawStr);

protected:

    bool verifyStaticContentExistence(const std::string & path);

    /**
    * @brief onClientURIReceived Virtual function called when the Client URI request
    *                         (ex. GET / HTTP/1.1) is available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool onClientURIReceived()
    {
        return true;
    }
    /**
    * @brief onClientHeadersReceived Virtual function called when the HTTP Client Headers are available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool onClientHeadersReceived()
    {
        return true;
    }
    /**
    * @brief onClientHeadersReceived Virtual function called when the whole client request
    *                             is available (GET/Options/Post Data).
    * @return true
    */
    virtual Mantids30::Network::Protocols::HTTP::Status::Codes onClientContentReceived()
    {
        return HTTP::Status::S_200_OK;
    }

    void * getThis() override { return this; }
    bool changeToNextParser() override;

    //ClientVars clientVars;

    /**
     * @brief resolveLocalFilePathFromURI2 Get the local and relative path from the URL, it also checks for transversal escape attempts and set the cache control and other things
     * @param defaultWebRootWithEndingSlash URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @param defaultFileToAppend  Append default suffix (eg. /index.html), default is not to append.
     * @param preventMappingExecutables Don't map the executable file as the response
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    bool resolveLocalFilePathFromURI2(std::string defaultWebRootWithEndingSlash, const std::list<std::pair<std::string, std::string>> &overlappedDirectories, LocalRequestedFileInfo * info, const std::string & defaultFileToAppend = "", const bool & preventMappingExecutables = false);
    /**
     * @brief resolveLocalFilePathFromURI0NE Only Get the local and relative path from the URL for non-existent file, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    static bool resolveLocalFilePathFromURI0NE(const std::string &uri, std::string sServerDir, LocalRequestedFileInfo * info);
    /**
     * @brief resolveLocalFilePathFromURI0E Only Get the local and relative path from the URL for existent file, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    static bool resolveLocalFilePathFromURI0E(const std::string &uri, std::string sServerDir, LocalRequestedFileInfo * info);

    /**
     * @brief setContentTypeByFileName Automatically set the content type depending the file extension from a preset
     * @param sFilePath filename string
     */
    bool detectContentTypeFromFilePath(const std::string & sFilePath);
    /**
     * @brief getCurrentFileExtension Get Current File Extension
     * @return File Extension
     */
    std::string getCurrentFileExtension() const;


private:
    bool changeToNextParserFromClientHeaders();
    bool changeToNextParserFromClientRequestLine();
    bool changeToNextParserFromClientContentData();

    bool streamServerHeaders();
    void prepareServerVersionOnURI();

    bool sendHTTPResponse();

    std::map<std::string, std::shared_ptr<Mantids30::Memory::Containers::B_MEM>> m_staticContentElements;

    bool m_isInvalidHTTPRequest = false;
    //Memory::Streams::WriteStatus m_answerBytes;

    std::string m_currentFileExtension;
    std::map<std::string,std::string> m_mimeTypes;
    void loadDefaultMIMETypes();
};

}

