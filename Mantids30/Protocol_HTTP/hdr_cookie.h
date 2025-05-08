#pragma once

#include <string>
#include "common_date.h"
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Headers {

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
     * @brief toSetCookieString Get the Cookie: string for the HTTP Header (without the Cookie:).
     * @param cookieName Name of the cookie
     * @return string value for the http header.
     */
    std::string toSetCookieString(const std::string & cookieName) const;
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


    // Default: the cookie will be secure. If you want to downgrade, do it manually.
    /**
     * @brief The cookie's value.
     */
    std::string value;

    /**
     * @brief The expiration date of the cookie.
     */
    Common::Date expires;

    /**
     * @brief The maximum age (in seconds) of the cookie.
     */
    uint32_t maxAge=UINT32_MAX;

    /**
     * @brief The domain associated with the cookie.
     */
    std::string domain;

    /**
     * @brief The path associated with the cookie.
     */
    std::string path;

    /**
     * @brief Indicates if the cookie is transmitted only over secure protocols.
     */
    bool secure=true;

    /**
     * @brief Indicates if the cookie is accessible only through HTTP (not via JavaScript).
     */
    bool httpOnly=true;

    /**
     * @brief The SameSite policy of the cookie.
     */
    eSameSitePolicy sameSitePolicy = HTTP_COOKIE_SAMESITE_STRICT;

private:
    std::pair<std::string, std::string> getVarNameAndValue(const std::string &var);

};

}}}}}

