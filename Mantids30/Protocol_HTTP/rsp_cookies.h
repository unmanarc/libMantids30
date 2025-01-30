#pragma once

#include <memory>
#include <string>
#include <map>
#include "hdr_cookie.h"
#include <Mantids30/Protocol_MIME/mime_sub_header.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Response {

class Cookies_ServerSide
{
public:
    Cookies_ServerSide();

    void putOnHeaders(MIME::MIME_Sub_Header * headers) const;

    std::string getCookieValueByName(const std::string & cookieName);
    std::shared_ptr<Headers::Cookie> getCookieByName(const std::string & cookieName);

    bool parseCookie(const std::string & cookie_str);
    bool addCookieVal(const std::string & cookieName, const Headers::Cookie & cookieValue);

    bool removeCookie(const std::string & cookieName);

    /**
     * @brief addClearCookie Add cookie with empty values (to clear the previous cookie)
     * @param cookieName cookie Name
     */
    void addClearSecureCookie(const std::string & cookieName);

private:
    std::map<std::string, std::shared_ptr<Headers::Cookie>> cookiesMap;

};
}}}}}

