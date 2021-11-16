#ifndef HTTP1SERVER_H
#define HTTP1SERVER_H

#include "httpv1_base.h"

#include "req_dataobjects.h"
#include "rsp_dataobjects.h"

#include "rsp_cookies.h"
#include "req_cookies.h"

// TODO: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Access-Control-Allow-Credentials

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

namespace CX2 { namespace Network { namespace HTTP {

enum HTTP_VarSource
{
    HTTP_VARS_POST,
    HTTP_VARS_GET
};

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
    }
    std::string sRealRelativePath;
    std::string sRealFullPath;
    bool isDir, isExecutable, isTransversal;
};

class HTTPv1_Server : public HTTPv1_Base
{
public:
    HTTPv1_Server(Memory::Streams::Streamable * sobject);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // REQUEST:
    Request::DataObjects getRequestActiveObjects();
    /**
     * @brief getRequestDataType Get Client Data Decodification Type
     * @return BIN/MIME/URL Options
     */
    Common::eContent_DataType getRequestDataType();
    /**
     * @brief setRequestDataContainer Set Container for request input
     * @param outStream container
     * @param deleteOutStream delete the container after usage.
     */
    void setRequestDataContainer(Memory::Streams::Streamable * dsIn, bool bDeleteAfter = false);
    /**
     * @brief getRequestDataContainer Request Data Container
     * @return
     */
    Memory::Streams::Streamable *getRequestDataContainer();
    /**
     * @brief getRequestVars POST/GET Variables introduced in the request.
     * @param source POST/GET option
     * @return
     */
    Memory::Abstract::Vars * getRequestVars(const HTTP_VarSource &source);
    /**
     * @brief getRequestVirtualHost Requested Virtual Host
     * @return virtual hostname string.
     */
    std::string getRequestVirtualHost() const;
    /**
     * @brief getRequestVirtualPort Requested Virtual port (Host: hostname:_port_)
     * @return port introduced in the Host option
     */
    uint16_t getRequestVirtualPort() const;
    /**
     * @brief getRequestURI Get Requested URI (eg. /index.html)
     * @return string containing the requested URI
     */
    std::string getRequestURI();
    /**
     * @brief getRequestCookie Get Request Cookie
     * @param sCookieName Cookie Name
     * @return Coookie Value.
     */
    std::string getRequestCookie(const std::string & sCookieName);
    /**
     * @brief getRequestContentType Get Request Payload Data Content Type
     * @return content type, eg. multipart/form-data (or any string sent by the http client)
     */
    std::string getRequestContentType();

    /**
     * @brief getClientHeaderOption Get Client Header Option Value By Option Name.
     * @param optionName Option Name
     * @return Option Value
     */
    std::string getClientHeaderOption(const std::string & optionName);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // RESPONSE:
    Response::DataObject getResponseActiveObjects();
    /**
     * @brief setServerTokens Set Server Header
     * @param serverTokens Server Header Product Name and Version (eg. MyLLS/5.0)
     */
    void setResponseServerName(const std::string &sServerName);


    /**
     * @brief getLocalFilePathFromURI2 Get the local and relative path from the URL, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param info Output with the Relative and full path of the requested resource, and other file information (dir/exec/etc)
     * @param defaultFileAppend  Append default suffix (eg. /index.html), default is not to append.
     * @param dontMapExecutables Don't map the executable file as the response
     * @return true if there is a positive resource that should be served, otherwise false.
     */
    bool getLocalFilePathFromURI2(std::string sServerDir, sLocalRequestedFileInfo * info, const std::string & defaultFileAppend = "", const bool & dontMapExecutables = false);

    /**
     * @brief getLocalFilePathFromURI (deprecated) Get the local and relative path from the URL, it also checks for transversal escape attempts
     * @param sServerDir URI
     * @param sRealRelativePath Output with the Relative path of the requested resource
     * @param sRealFullPath Output with the Full path of the requested resource
     * @param defaultFileAppend Append default suffix (eg. /index.html), default is not to append.
     * @param isDir Output parameter to check if the requested resource is a directory
     * @return true if there is a resource, false if not
     bool getLocalFilePathFromURI(const std::string &sServerDir, std::string *sRealRelativePath, std::string *sRealFullPath, const std::string & defaultFileAppend = "", bool * isDir = nullptr);
*/
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
     * @brief setResponseDataStreamer Set the container used for transmiting data.
     * @param outStream stream used (or nullptr to default empty streamer)
     * @param deleteOutStream delete the container after usage.
     */
    void setResponseDataStreamer(Memory::Streams::Streamable * dsOut, bool bDeleteAfter = false);
    /**
     * @brief getResponseDataStreamer Get the response data streamer
     * @return
     */
    Memory::Streams::Streamable *getResponseDataStreamer();
    /**
     * @brief getResponseTransmissionStatus Get the response transmitted byte count after the request was completed.
     * @return
     */
    Memory::Streams::Status getResponseTransmissionStatus() const;
    /**
     * @brief addCookieClearSecure Set Response Secure Cookie (Secure,httpOnly,SameSite) as delete cookie
     * @param cookieName
     * @return
     */
    void addCookieClearSecure(const std::string &cookieName);
    /**
     * @brief setResponseSecureCookie Set Response Secure Cookie (Secure,httpOnly,SameSite)
     * @param cookieName
     * @param cookieValue
     * @param uMaxAge
     * @return
     */
    bool setResponseSecureCookie(const std::string &cookieName, const std::string & cookieValue, const uint32_t & uMaxAge );
    /**
     * @brief setResponseInsecureCookie Set HTTP Simple Cookie (don't use, very insecure)
     * @param cookieName name of the cookie variable
     * @param cookieValue value of the cookie.
     * @return false if already exist
     */
    bool setResponseInsecureCookie( const std::string &sCookieName, const std::string & sCookieValue );
    /**
     * @brief setResponseCookie Set HTTP Cookie with full options
     * @param cookieName name of the cookie variable
     * @param cookieValue value of the cookie.
     * @return false if already exist
     */
    bool setResponseCookie(const std::string &sCookieName, const Headers::Cookie &sCookieValue );
    /**
     * @brief streamResponse Stream Response to data streamer container (may copy bytes into a container, don't use for massive data transfers)
     * @return Status of the Operation
     */
    Memory::Streams::Status streamResponse(Memory::Streams::Streamable * source);
    /**
     * @brief setResponseRedirect Redirect site to another URL
     * @param location URL string
     */
    Response::StatusCode setResponseRedirect(const std::string & location, bool temporary = true);
    /**
     * @brief setResponseContentType Set Response Content Type
     * @param contentType Content Type (eg. application/json, text/html)
     * @param bNoSniff tell the browser do not sniff/guess the content type
     */
    void setResponseContentType(const std::string & contentType, bool bNoSniff = true);

    /**
     * @brief getCurrentFileExtension Get Current File Extension
     * @return File Extension
     */
    std::string getCurrentFileExtension() const;

    /**
     * @brief getContentType Get the generated content type (eg. text/html)
     * @return content type string in lowercase.
     */
    std::string getContentType() const;

    // SECURITY OPTIONS:
    Headers::Security::XFrameOpts getResponseSecurityXFrameOpts() const;
    void setResponseSecurityXFrameOpts(const Headers::Security::XFrameOpts &value);

    Headers::Security::XSSProtection getResponseSecurityXSSProtection() const;
    void setResponseSecurityXSSProtection(const Headers::Security::XSSProtection &value);

    Headers::Security::HSTS getResponseSecurityHSTS() const;
    void setResponseSecurityHSTS(const Headers::Security::HSTS &value);

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

    void addStaticContent(const std::string & path, CX2::Memory::Containers::B_MEM * contentElement);
    void setStaticContentElements(const std::map<std::string, CX2::Memory::Containers::B_MEM *> &value);

    static std::string htmlEncode(const std::string& rawStr);

protected:

    bool verifyStaticContentExistence(const std::string & path);

    /**
    * @brief processClientURI Virtual function called when the Client URI request
    *                         (ex. GET / HTTP/1.1) is available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool processClientURI();
    /**
    * @brief processClientOptions Virtual function called when the Client Option are available.
    * @return true continue with the parsing / false end parsing and close connection.
    */
    virtual bool processClientOptions();
    /**
    * @brief processClientOptions Virtual function called when the whole client request
    *                             is available (GET/Options/Post Data).
    * @return true
    */
    virtual Response::StatusCode processClientRequest();

    void * getThis() override { return this; }
    bool changeToNextParser() override;

    char remotePairAddress[INET6_ADDRSTRLEN];

private:
    bool changeToNextParserOnClientHeaders();
    bool changeToNextParserOnClientRequest();
    bool changeToNextParserOnClientContentData();
    bool streamServerHeaders(Memory::Streams::Status &wrStat);
    void prepareServerVersionOnURI();

    void prepareServerVersionOnOptions();
    void parseHostOptions();

    bool answer(Memory::Streams::Status &wrStat);

    std::map<std::string,CX2::Memory::Containers::B_MEM *> staticContentElements;

    Response::Cookies_ServerSide setCookies;
    Headers::Security::XFrameOpts secXFrameOpts;
    Headers::Security::XSSProtection secXSSProtection;
    Headers::Security::HSTS secHSTS;
    Headers::CacheControl cacheControl;

    bool badAnswer;
    Memory::Streams::Status ansBytes;
    uint16_t virtualPort;

    std::string authenticate;
    std::string virtualHost;
    std::string contentType;
    std::string currentFileExtension;
    bool bNoSniff, isSecure, includeServerDate;
    std::map<std::string,std::string> mimeTypes;
};

}}}

#endif // HTTP1SERVER_H
