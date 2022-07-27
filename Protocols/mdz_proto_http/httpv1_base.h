#ifndef HTTP1BASE_H
#define HTTP1BASE_H

#include <mdz_mem_vars/parser.h>
#include <mdz_mem_vars/vars.h>
#include <mdz_proto_mime/mime_sub_header.h>

#include "common_content.h"
#include "hdr_cachecontrol.h"
#include "hdr_sec_hsts.h"
#include "hdr_sec_xssprotection.h"
#include "hdr_sec_xframeopts.h"
#include "req_requestline.h"
#include "req_cookies.h"
#include "rsp_cookies.h"
#include "rsp_status.h"

#define HTTP_PRODUCT_VERSION_MAJOR 0
#define HTTP_PRODUCT_VERSION_MINOR 4

namespace Mantids { namespace Protocols { namespace HTTP {


class HTTPv1_Base : public Memory::Streams::Parser
{
public:
    enum HTTP_VarSource
    {
        HTTP_VARS_POST,
        HTTP_VARS_GET
    };

    struct Security
    {
        Security()
        {
            bNoSniffContentType = false;
        }
        Headers::Security::XFrameOpts XFrameOpts;
        Headers::Security::XSSProtection XSSProtection;
        Headers::Security::HSTS HSTS;
        bool bNoSniffContentType;
    };

    struct BasicAuth
    {
        BasicAuth()
        {
            bEnabled = false;
        }

        void setAuthentication(const std::string &user, const std::string &pass)
        {
            bEnabled = true;
            this->user = user;
            this->pass = pass;
        }

        std::string user,pass;
        bool bEnabled;
    };

    struct Request
    {
        Request()
        {
            virtualPort = 80;
        }


        // HTTP Quick Access Functions:
        /**
         * @brief getVars Get Vars
         * @param source
         * @return
         */
        Memory::Abstract::Vars *getVars(const HTTP_VarSource &source)
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
        /**
         * @brief getCookie Get Cookie
         * @param sCookieName
         * @return
         */
        std::string getCookie(const std::string &sCookieName)
        {
            MIME::MIME_HeaderOption * cookiesSubVars = headers.getOptionByName("Cookie");
            if (!cookiesSubVars) return "";
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

        /**
         * @brief clientRequest - URL Request (Request type, URL, GET Vars, and HTTP version)
         */
        Mantids::Protocols::HTTP::Request::RequestLine requestLine;
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
        uint16_t virtualPort;
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
            security.bNoSniffContentType = bNoSniff;
        }
        /**
         * @brief setResponseDataStreamer Set the container used for transmiting data.
         * @param outStream stream used (or nullptr to default empty streamer)
         * @param deleteOutStream delete the container after usage.
         */
        void setDataStreamer(Memory::Streams::StreamableObject * dsOut, bool bDeleteAfter = false)
        {
            if (dsOut == nullptr)
            {
                // Set default headers (lost previous ones):
                headers.remove("Last-Modified");
                cacheControl.setDefaults();
                cacheControl.setOptionNoCache(true);
                cacheControl.setOptionNoStore(true);
                cacheControl.setOptionMustRevalidate(true);
                setContentType("",false);
            }
            content.setStreamableObj(dsOut,bDeleteAfter);
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
        Mantids::Protocols::HTTP::Status status;
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

    Request clientRequest;
    Response serverResponse;
protected:
    virtual bool initProtocol() override;
    virtual void endProtocol() override;

    virtual void * getThis()=0;
    virtual bool changeToNextParser()  override = 0;

private:
    void setInternalProductVersion(const std::string & prodName, const std::string & extraInfo, const uint32_t &versionMajor = HTTP_PRODUCT_VERSION_MAJOR, const uint32_t &versionMinor = HTTP_PRODUCT_VERSION_MINOR);
};

}}}

#endif // HTTP1BASE_H
