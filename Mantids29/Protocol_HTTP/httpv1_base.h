#pragma once

#include <Mantids29/Memory/parser.h>
#include <Mantids29/Memory/vars.h>
#include <Mantids29/Protocol_MIME/mime_sub_header.h>
#include <memory>
#include <netinet/in.h>
#include <string>

#include "Mantids29/Memory/streamablejson.h"
#include "common_content.h"
#include "hdr_cachecontrol.h"
#include "hdr_sec_hsts.h"
#include "hdr_sec_xssprotection.h"
#include "hdr_sec_xframeopts.h"
#include "req_requestline.h"
//#include "req_cookies.h"
#include "rsp_cookies.h"
#include "rsp_status.h"

#define HTTP_PRODUCT_VERSION_MAJOR 0
#define HTTP_PRODUCT_VERSION_MINOR 4

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP {


class HTTPv1_Base : public Memory::Streams::Parser
{
public:
    enum HTTP_VarSource
    {
        HTTP_VARS_POST,
        HTTP_VARS_GET
    };

    /**
     * @brief A struct representing security settings for the web server.
     *
     * This struct is used to store various security settings for the web server, including X-Frame-Options, XSS protection,
     * and HSTS (HTTP Strict Transport Security). These settings help protect against various security threats, such as
     * clickjacking and cross-site scripting attacks.
     */
    struct Security
    {
        Headers::Security::XFrameOpts XFrameOpts;   ///< The X-Frame-Options setting.
        Headers::Security::XSSProtection XSSProtection; ///< The XSS protection setting.
        Headers::Security::HSTS HSTS;               ///< The HSTS (HTTP Strict Transport Security) setting.
        bool disableNoSniffContentType = false;     ///< Whether or not to disable the no-sniff content type header.
    };

    /**
     * @brief A struct representing basic authentication settings.
     *
     * This struct is used to store basic authentication settings, which include a username and password. These settings can
     * be used to authenticate users before granting them access to certain resources.
     */
    struct BasicAuth
    {
        /**
         * @brief Sets the username and password for basic authentication.
         *
         * This function sets the username and password for basic authentication. It also sets the bEnabled flag to true,
         * indicating that basic authentication is now enabled.
         *
         * @param username The username to use for authentication.
         * @param password The password to use for authentication.
         */
        void setAuthentication(const std::string& username, const std::string& password)
        {
            isEnabled = true;
            this->username = username;
            this->password = password;
        }

        std::string username; ///< The username to use for basic authentication.
        std::string password; ///< The password to use for basic authentication.
        bool isEnabled = false;       ///< Whether basic authentication is enabled or not.
    };

    struct Request
    {
        // HTTP Quick Access Functions:
        /**
         * @brief getVars Get Vars
         * @param source
         * @return
         */
        std::shared_ptr<Memory::Abstract::Vars> getVars(const HTTP_VarSource &source)
        {
            switch (source)
            {
            case HTTP_VARS_POST:
                return content.postVars();
            case HTTP_VARS_GET:
                return requestLine.urlVars();
            }
            return nullptr;
        }        
        MIME::MIME_HeaderOption * getCookies()
        {
            return headers.getOptionByName("Cookie");
        }

        /**
         * @brief getJSONContent Get the JSON Content if any...
         * @return object to the unparsed json streamer if JSON was requested (or set as request)
         */
        std::shared_ptr<Mantids29::Memory::Streams::StreamableJSON> getJSONStreamerContent()
        {
            if (content.getContainerType() == Common::Content::CONTENT_TYPE_JSON)
            {
                return content.getJSONVars();
            }
            return nullptr;
        }

        std::multimap<std::string, std::string> getAllCookies()
        {
            MIME::MIME_HeaderOption *cookiesSubVars = headers.getOptionByName("Cookie");
            if (!cookiesSubVars)
                return {};
            return cookiesSubVars->getAllSubVars();
        }

        /**
         * @brief getCookie Get Cookie
         * @param sCookieName
         * @return
         */
        std::string getCookie(const std::string &sCookieName)
        {
            MIME::MIME_HeaderOption * cookiesSubVars = headers.getOptionByName("Cookie");
            if (!cookiesSubVars)
                return "";
            // TODO: mayus
            return cookiesSubVars->getSubVar(sCookieName);
        }
        /**
         * @brief getContentType Get Content Type
         * @return
         */
        std::string getContentType()
        {
            return headers.getOptionRawStringByName("Content-Type");
        }
        /**
         * @brief getURI Get URI
         * @return
         */
        std::string getURI()
        {
            return requestLine.getURI();
        }
        /**
         * @brief getHeaderOption Get Client Header Option Value By Option Name.
         * @param optionName Option Name
         * @return Option Value
         */
        std::string getHeaderOption(const std::string & optionName)
        {
            return headers.getOptionRawStringByName(optionName);
        }

        struct NetworkClientInfo {
            char REMOTE_ADDR[INET6_ADDRSTRLEN] = "";
            bool isSecure = false;
            std::string tlsCommonName = "";
        };

        NetworkClientInfo networkClientInfo;

        /**
         * @brief clientRequest - URL Request (Request type, URL, GET Vars, and HTTP version)
         */
        Mantids29::Network::Protocols::HTTP::Request::RequestLine requestLine;
        /**
         * @brief content - Content Data.
         */
        Common::Content content;
        /**
         * @brief headers - Options Values.
         */
        MIME::MIME_Sub_Header headers;
        /**
         * @brief basicAuth Authentication Information
         */
        BasicAuth basicAuth;
        /**
         * @brief userAgent HTTP Client User Agent ID
         */
        std::string userAgent;
        /**
         * @brief virtualHost Requested Virtual Host
         */
        std::string virtualHost;
        /**
         * @brief virtualPort Requested Virtual Port
         */
        uint16_t virtualPort = 80;
    };
    struct Response
    {
        /**
         * @brief setResponseContentType Set Response Content Type
         * @param contentType Content Type (eg. application/json, text/html)
         * @param bNoSniff tell the browser do not sniff/guess the content type
         */
        void setContentType(const std::string & contentType, bool bNoSniff = true)
        {
            this->contentType = contentType;
            security.disableNoSniffContentType = bNoSniff;
        }

        /**
         * @brief Set the container used for transmitting data.
         * @param dataStream The stream used, or nullptr to use the default empty streamer.
         */
        void setDataStreamer(std::shared_ptr<Memory::Streams::StreamableObject> dataStream)
        {
            if (!dataStream)
            {
                // Set default headers (lost previous ones):
                headers.remove("Last-Modified");
                cacheControl.setDefaults();
                cacheControl.setOptionNoCache(true);
                cacheControl.setOptionNoStore(true);
                cacheControl.setOptionMustRevalidate(true);
                setContentType("", false);
            }
            content.setStreamableObj(dataStream);
        }
        /**
         * @brief addCookieClearSecure Set Response Secure Cookie (Secure,httpOnly,SameSite) as delete cookie
         * @param cookieName
         * @return
         */
        void addCookieClearSecure(const std::string &cookieName)
        {
            cookies.addClearSecureCookie(cookieName);
        }
        /**
         * @brief setSecureCookie Set Response Secure Cookie (Secure,httpOnly,SameSite)
         * @param cookieName
         * @param cookieValue
         * @param uMaxAge
         * @return
         */
        bool setSecureCookie(const std::string &cookieName, const std::string & cookieValue, const uint32_t & uMaxAge )
        {
            Headers::Cookie val;
            val.setValue(cookieValue);
            val.setSecure(true);
            val.setHttpOnly(true);
            val.setExpirationFromNow(uMaxAge);
            val.setMaxAge(uMaxAge);
            val.setSameSite(Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT);
            return setCookie(cookieName,val);
        }
        /**
         * @brief setInsecureCookie Set HTTP Simple Cookie (don't use, very insecure)
         * @param cookieName name of the cookie variable
         * @param cookieValue value of the cookie.
         * @return false if already exist
         */
        bool setInsecureCookie( const std::string &sCookieName, const std::string & sCookieValue )
        {
            Headers::Cookie val;
            val.setValue(sCookieValue);
            return setCookie(sCookieName,val);
        }
        /**
         * @brief setCookie Set HTTP Cookie with full options
         * @param cookieName name of the cookie variable
         * @param cookieValue value of the cookie.
         * @return false if already exist
         */
        bool setCookie(const std::string &sCookieName, const Headers::Cookie &sCookieValue )
        {
            return cookies.addCookieVal(sCookieName,sCookieValue);
        }

        /**
         * @brief code Response - Server code response. (HTTP Version, Response code, message)
         */
        Mantids29::Network::Protocols::HTTP::Status status;
        /**
         * @brief content - Content Data.
         */
        Common::Content content;
        /**
         * @brief headers - Options Values.
         */
        MIME::MIME_Sub_Header headers;        
        /**
         * @brief security Response Security
         */
        Security security;
        /**
         * @brief cookies Set-Cookies cookies...
         */
        HTTP::Response::Cookies_ServerSide cookies;
        /**
         * @brief cacheControl Cache Control
         */
        Headers::CacheControl cacheControl;
        /**
         * @brief sWWWAuthenticateRealm WWW-Authenticate Realm String (if not empty, authentication is requested to the client)
         */
        std::string sWWWAuthenticateRealm;
        /**
         * @brief contentType Response Content Type
         */
        std::string contentType;
    };

    HTTPv1_Base(bool clientMode, Memory::Streams::StreamableObject *sobject);
    virtual ~HTTPv1_Base()  override {}

    Request m_clientRequest;
    Response m_serverResponse;
protected:
    virtual bool initProtocol() override;
    virtual void endProtocol() override;

    virtual void * getThis()=0;
    virtual bool changeToNextParser()  override = 0;

private:
    void setInternalProductVersion(const std::string & prodName, const std::string & extraInfo, const uint32_t &versionMajor = HTTP_PRODUCT_VERSION_MAJOR, const uint32_t &versionMinor = HTTP_PRODUCT_VERSION_MINOR);
};

}}}}

