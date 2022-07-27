#ifndef HTTP1SERVER_H
#define HTTP1SERVER_H

#include "httpv1_base.h"

// TODO: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Access-Control-Allow-Credentials

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

namespace Mantids { namespace Protocols { namespace HTTP {


class HTTPv1_Server : public HTTPv1_Base
{
public:
    struct sLocalRequestedFileInfo
    {
        sLocalRequestedFileInfo()
        {
            reset();
        }
        void reset()
        {
            sRealRelativePath="";
            sRealFullPath="";
            isDir = false;
            isExecutable = false;
            isTransversal = false;
            pathExist = false;
        }
        std::string sRealRelativePath;
        std::string sRealFullPath;
        bool isDir, isExecutable, isTransversal, pathExist;
    };


    struct sClientVars
    {
        sClientVars()
        {
            memset(REMOTE_ADDR,0,sizeof(REMOTE_ADDR));
        }
        // Host Information:
        char REMOTE_ADDR[INET6_ADDRSTRLEN];

        // Proceced information:
        Memory::Abstract::Vars *VARS_GET, *VARS_POST;
        MIME::MIME_HeaderOption * VARS_COOKIES;
    };

    HTTPv1_Server(Memory::Streams::StreamableObject * sobject);


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // RESPONSE:
    /**
     * @brief setServerTokens Set Server Header
     * @param serverTokens Server Header Product Name and Version (eg. MyLLS/5.0)
     */
    void setResponseServerName(const std::string &sServerName);
    /**
     * @brief getLocalFilePathFromURI2 Get the local and relative path from the URL, it also checks for transversal escape attempts and set the cache control and other things
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @param defaultFileAppend  Append default suffix (eg. /index.html), default is not to append.
     * @param dontMapExecutables Don't map the executable file as the response
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    bool getLocalFilePathFromURI2(std::string sServerDir, sLocalRequestedFileInfo * info, const std::string & defaultFileAppend = "", const bool & dontMapExecutables = false);
    /**
     * @brief getLocalFilePathFromURI0NE Only Get the local and relative path from the URL for non-existent file, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    static bool getLocalFilePathFromURI0NE(const std::string &uri, std::string sServerDir, sLocalRequestedFileInfo * info);
    /**
     * @brief getLocalFilePathFromURI0E Only Get the local and relative path from the URL for existent file, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    static bool getLocalFilePathFromURI0E(const std::string &uri, std::string sServerDir, sLocalRequestedFileInfo * info);
    /**
     * @brief setContentTypeByFileName Automatically set the content type depending the file extension from a preset
     * @param sFilePath filename string
     */
    bool setResponseContentTypeByFileExtension(const std::string & sFilePath);
    /**
     * @brief addFileExtensionMimeType Add/Replace File Extension to Mime Content Type Association (no Thread-Safe, must be done before start the server)
     * @param ext extension (important!!: should be in lowercase)
     * @param content type
     */
    void addResponseContentTypeFileExtension(const std::string & ext, const std::string & type);

    /**
     * @brief getResponseDataStreamer Get the response data streamer
     * @return
     */
    Memory::Streams::StreamableObject *getResponseDataStreamer();
    /**
     * @brief getResponseTransmissionStatus Get the response transmitted byte count after the request was completed.
     * @return
     */
    Memory::Streams::StreamableObject::Status getResponseTransmissionStatus() const;

    /**
     * @brief streamResponse Stream Response to data streamer container (may copy bytes into a container, don't use for massive data transfers)
     * @return Status of the Operation
     */
    Memory::Streams::StreamableObject::Status streamResponse(Memory::Streams::StreamableObject * source);
    /**
     * @brief setResponseRedirect Redirect site to another URL
     * @param location URL string
     */
    Mantids::Protocols::HTTP::Status::eRetCode setResponseRedirect(const std::string & location, bool temporary = true);
    /**
     * @brief getCurrentFileExtension Get Current File Extension
     * @return File Extension
     */
    std::string getCurrentFileExtension() const;

    void setResponseIncludeServerDate(bool value);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OTHER FUNCTIONS:
    /**
     * @brief setRemotePairAddress Internal function to set the remote pair address... (don't use)
     * @param value ip address
     */
    void setRemotePairAddress(const char * value);

    bool getIsSecure() const;
    void setIsSecure(bool value);

    void addStaticContent(const std::string & path, Mantids::Memory::Containers::B_MEM * contentElement);
    void setStaticContentElements(const std::map<std::string, Mantids::Memory::Containers::B_MEM *> &value);

    static std::string htmlEncode(const std::string& rawStr);


protected:

    bool verifyStaticContentExistence(const std::string & path);

    /**
    * @brief procHTTPClientURI Virtual function called when the Client URI request
    *                         (ex. GET / HTTP/1.1) is available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool procHTTPClientURI()
    {
        return true;
    }
    /**
    * @brief procHTTPClientHeaders Virtual function called when the HTTP Client Headers are available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool procHTTPClientHeaders()
    {
        return true;
    }
    /**
    * @brief procHTTPClientHeaders Virtual function called when the whole client request
    *                             is available (GET/Options/Post Data).
    * @return true
    */
    virtual Mantids::Protocols::HTTP::Status::eRetCode procHTTPClientContent()
    {
        return HTTP::Status::S_200_OK;
    }

    void * getThis() override { return this; }
    bool changeToNextParser() override;

    sClientVars clientVars;

private:
    void fillRequestDataStruct();
    bool changeToNextParserOnClientHeaders();
    bool changeToNextParserOnClientRequest();
    bool changeToNextParserOnClientContentData();
    bool streamServerHeaders(Memory::Streams::StreamableObject::Status &wrStat);
    void prepareServerVersionOnURI();

    void prepareServerVersionOnOptions();
    void parseHostOptions();

    bool answer(Memory::Streams::StreamableObject::Status &wrStat);

    std::map<std::string,Mantids::Memory::Containers::B_MEM *> staticContentElements;

    bool badAnswer;
    Memory::Streams::StreamableObject::Status ansBytes;

    std::string currentFileExtension;
    bool isSecure, includeServerDate;
    std::map<std::string,std::string> mimeTypes;
};

}}}

#endif // HTTP1SERVER_H
