#ifndef HTTP1CLIENT_H
#define HTTP1CLIENT_H

#include "httpv1_base.h"

#include "rsp_cookies.h"
#include "req_cookies.h"
#include "hdr_sec_xframeopts.h"
#include "hdr_sec_xssprotection.h"
#include "hdr_sec_hsts.h"

// TODO: https://en.wikipedia.org/wiki/Media_type
// TODO: cuando el request para doh5 este listo, pre-procesar primero el request y luego recibir los datos.
// TODO: post data? <<< IMPORTANT.
// TODO: reuse the connection?...
// TODO: header: :scheme:https (begins with :)

namespace CX2 { namespace Network { namespace HTTP {

class HTTPv1_Client : public HTTPv1_Base
{
public:
    HTTPv1_Client(Memory::Streams::Streamable * sobject);
    /**
     * @brief setClientRequest Set client request
     * @param hostName host name
     * @param uriPath requested Uniform Resource Indetifier Path
     */
    void setClientRequest(const std::string & hostName, const std::string & uriPath);
    /**
     * @brief setDontTrackFlag Set Don't track flag
     * @param dnt true: don't track
     */
    void setDontTrackFlag(bool dnt = true);
    /**
     * @brief setReferer Set Referer URL
     * @param refererURL referer URL
     */
    void setReferer(const std::string & refererURL);
    /**
     * @brief addCookieValue Add cookie.
     * @param cookieName Cookie Name
     * @param cookieVal Coookie Value
     */
    void addCookie(const std::string & cookieName, const std::string & cookieVal);
    /**
     * @brief setClientUserAgent Set Client User Agent
     * @param userAgent User Agent String (eg. Wgetty/1.1)
     */
    void setClientUserAgent(const std::string &userAgent);
    /**
     * @brief setAuthenticationBasic
     * @param user
     * @param pass
     */
    void setBasicAuthentication(const std::string &user, const std::string &pass);
    /**
     * @brief getServerCookies Get the server cookies container with the information of received cookies
     * @return server cookies container.
     */
    Response::Cookies_ServerSide * getServerCookies();
    /**
     * @brief getServerContentType Get Server Content Type
     * @return Content Type
     */
    std::string getServerContentType() const;
    /**
     * @brief getSecurityNoSniffContentType Get if No-Sniff option was sent
     * @return
     */
    bool getSecurityNoSniffContentType() const;
    /**
     * @brief getSecXFrameOpts Get Security XFrame Options
     * @return
     */
    Headers::Security::XFrameOpts getSecXFrameOpts() const;
    /**
     * @brief getSecXSSProtection Get XSS Protection Header from server
     * @return
     */
    Headers::Security::XSSProtection getSecXSSProtection() const;
    /**
     * @brief getSecHSTS Get HSTS Policy...
     * @return
     */
    Headers::Security::HSTS getSecHSTS() const;

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
    Memory::Streams::Parsing::SubParser *parseHeaders2TransmitionMode();

    bool streamClientHeaders(Memory::Streams::Status &wrStat);

    Request::Cookies_ClientSide clientCookies;
    Response::Cookies_ServerSide serverCookies;
    Headers::Security::XFrameOpts secXFrameOpts;
    Headers::Security::HSTS secHSTS;
    Headers::Security::XSSProtection secXSSProtection;

    std::string serverContentType;
    bool securityNoSniffContentType;
};

}}}

#endif // HTTP1CLIENT_H
