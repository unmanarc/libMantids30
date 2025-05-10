#pragma once

#include "httpv1_base.h"
#include "req_cookies.h"

// TODO: https://en.wikipedia.org/wiki/Media_type
// TODO: cuando el request para doh5 este listo, pre-procesar primero el request y luego recibir los datos.
// TODO: post data? <<< IMPORTANT.
// TODO: reuse the connection?...
// TODO: header: :scheme:https (begins with :)

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP {

class HTTPv1_Client : public HTTPv1_Base
{
public:
    HTTPv1_Client(std::shared_ptr<Memory::Streams::StreamableObject>  sobject);
    /**
     * @brief setClientRequest Set client request
     * @param hostName host name
     * @param uriPath requested Uniform Resource Indetifier Path
     */
    void setClientRequest(const std::string & hostName, const std::string & uriPath);

    struct PostMIMERequest {
        std::shared_ptr<Protocols::MIME::MIME_Message> postVars;
        std::shared_ptr<HTTP::URLVars> urlVars;
    };
    /**
     * @brief prepareRequestAsPostMIME Prepare a POST MIME Request (by default we use GET)
     * @param hostName
     * @param uriPath
     */
    PostMIMERequest prepareRequestAsPostMIME( const std::string & hostName, const std::string & uriPath );

    struct PostURLRequest {
        std::shared_ptr<HTTP::URLVars> postVars;
        std::shared_ptr<HTTP::URLVars> urlVars;
    };
    /**
     * @brief prepareRequestAsPostURL Prepare a POST URL Request (by default we use GET)
     * @param hostName
     * @param uriPath
     */
    PostURLRequest prepareRequestAsPostURL( const std::string & hostName, const std::string & uriPath );

    /**
     * @brief getResponseHeader Get Response Header from the server as string
     * @param headerName Header Name
     * @return string with the header value
     */
    std::string getResponseHeader(const std::string & headerName);
    /**
     * @brief setReferer Set Referer URL
     * @param refererURL referer URL
     */
    void setReferer(const std::string & refererURL);
    /**
     * @brief addURLVar Add URL GET Variable
     * @param varName Variable Name
     * @param varValue Variable Value
     */
    void addURLVar(const std::string & varName, const std::string & varValue);
    /**
     * @brief addCookieValue Add cookie.
     * @param cookieName Cookie Name
     * @param cookieVal Coookie Value
     */
    void addCookie(const std::string & cookieName, const std::string & cookieVal);
    /**
     * @brief setAuthenticationBasic
     * @param user
     * @param pass
     */
    void setBasicAuthentication(const std::string &user, const std::string &pass);
    /**
     * @brief getServerContentType Get Server Content Type
     * @return Content Type
     */
    std::string getServerContentType() const;

protected:
    bool initProtocol() override;

    void * getThis() override { return this; }
    bool changeToNextParser() override;

    /**
      // TODO:
     * @brief onStart emit this signal when download starts
     */
    virtual void onStart() {}
    /**
     * // TODO:
     * @brief onProgress emit progress percentage when data is received.
     * @param progressPct percentage using scale /10000
     */
    virtual void onProgress(const uint16_t &progressPct, const uint64_t &progressBytes, const uint64_t &progressExpectedBytes) {}
    /**
      // TODO:
     * @brief onFinished emit this signal when connection is ended.
     */
    virtual void onFinished() {}
private:
    void parseHeaders2ServerCookies();
    Memory::Streams::SubParser *parseHeaders2TransmitionMode();

    bool streamClientHeaders(Memory::Streams::WriteStatus &wrStat);

    HTTP::Request::Cookies_ClientSide m_clientCookies;

    std::string m_serverContentType;
};

}}}}

