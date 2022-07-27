#ifndef HTTP_COOKIE_VALUE_H
#define HTTP_COOKIE_VALUE_H

#include <string>
#include "common_date.h"

namespace Mantids { namespace Protocols { namespace HTTP { namespace Headers {

class Cookie
{
public:

    /**
     * @brief The eSameSitePolicy enum
     */
    enum eSameSitePolicy {
        HTTP_COOKIE_SAMESITE_NONE = 0,
        HTTP_COOKIE_SAMESITE_LAX = 1,
        HTTP_COOKIE_SAMESITE_STRICT = 2
    };

    /**
     * @brief Cookie Constructor
     */
    Cookie();

    /**
     * @brief setDefaults Set Cookie To Default Values (very relaxed ones)
     */
    void setDefaults();

    /**
     * @brief toSetCookieString Get the Cookie: string for the HTTP Header (without the Cookie:).
     * @param cookieName Name of the cookie
     * @return string value for the http header.
     */
    std::string toSetCookieString(const std::string & cookieName);
    /**
     * @brief fromSetCookieString Parse cookie from the HTTP Cookie: string  (after the Cookie:)
     * @param setCookieValue cookie value
     * @param cookieName cookie name output, if value not found, the cookie Name is not filled.
     */
    void fromSetCookieString(const std::string & setCookieValue, std::string * cookieName);

    ///////////////////////////////////////////////////////////////////////////////

    /**
     * @brief getExpiration Get Expiration
     * @return Unix Time for the Expiration, or 0 if is a session cookie
     */
    time_t getExpiration() const;
    /**
     * @brief setAsTransientCookie set the cookie as a session/trasient cookie
     */
    void setAsTransientCookie();
    /**
     * @brief setExpiration Set Expiration to UNIX date
     * @param exp unix date
     */
    void setExpiration(const time_t& exp);
    /**
     * @brief setExpirationFromNow Set Expiration in seconds from now
     * @param seconds seconds from now
     */
    void setExpirationFromNow(const uint32_t& seconds);

    /**
     * @brief getValue Get the cookie content value
     * @return cookie content value
     */
    std::string getValue() const;
    /**
     * @brief setValue Set the cookie content value
     * @param value cookie content value
     */
    void setValue(const std::string &value);

    /**
     * @brief getMaxAge Get Max Age (Maximum time that the cookie can survive)
     * @return Max Age in seconds
     */
    uint32_t getMaxAge() const;
    /**
     * @brief setMaxAge Set Max Age (Maximum time that the cookie can survive)
     * @param value Max Age in seconds, or std::numeric_limits<uint32_t>::max() to disable this field (default)
     */
    void setMaxAge(const uint32_t &value);

    /**
     * @brief getDomain Get Cookie Domain
     * @return cookie domain name
     */
    std::string getDomain() const;
    /**
     * @brief setDomain Set cookie domain
     * @param value cookie domain name
     */
    void setDomain(const std::string &value);

    /**
     * @brief getPath Get the cookie path
     * @return cookie path
     */
    std::string getPath() const;
    /**
     * @brief setPath Set the cookie path
     * @param value cookie path
     */
    void setPath(const std::string &value);

    /**
     * @brief isHttpOnly Get if the cookie is HTTP Only  (javascript can't read this)
     * @return true if httpOnly flag is up, false otherwise
     */
    bool isHttpOnly() const;
    /**
     * @brief setHttpOnly Set the HTTP Only flag
     * @param value http only flag
     */
    void setHttpOnly(bool value);

    /**
     * @brief isSecure Get the cookie secure flag (will be only available in https)
     * @return true if the secure flag is on
     */
    bool isSecure() const;
    /**
     * @brief setSecure Set the cookie secure flag (will be only available in https)
     * @param value secure flag option
     */
    void setSecure(bool value);

    /**
     * @brief getSameSite Get the same site policy
     * @return same site policy
     */
    eSameSitePolicy getSameSite() const;
    /**
     * @brief setSameSite Set the same site policy
     * @param value same site policy
     */
    void setSameSite(const eSameSitePolicy &value);

private:
    std::pair<std::string, std::string> getVarNameAndValue(const std::string &var);

    std::string value;
    Common::Date expires;
    uint32_t max_age;
    std::string domain;
    std::string path;
    bool secure,httpOnly;
    eSameSitePolicy sameSite;
};

}}}}

#endif // HTTP_COOKIE_VALUE_H
