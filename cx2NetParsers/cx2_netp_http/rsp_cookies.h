#ifndef HTTP_SETCOOKIES_H
#define HTTP_SETCOOKIES_H

#include <string>
#include <map>
#include "hdr_cookie.h"
#include <cx2_netp_mime/mime_sub_header.h>
#include <stdint.h>

namespace CX2 { namespace Network { namespace HTTP { namespace Response {

class Cookies_ServerSide
{
public:
    Cookies_ServerSide();
    ~Cookies_ServerSide();

    void putOnHeaders(MIME::MIME_Sub_Header * headers) const;

    std::string getCookieValueByName(const std::string & cookieName);
    Headers::Cookie *getCookieByName(const std::string & cookieName);

    bool parseCookie(const std::string & cookie_str);
    bool addCookieVal(const std::string & cookieName, const Headers::Cookie & cookieValue);

    /**
     * @brief addClearCookie Add cookie with empty values (to clear the previous cookie)
     * @param cookieName cookie Name
     */
    void addClearSecureCookie(const std::string & cookieName);

private:
    std::map<std::string,Headers::Cookie *> cookiesMap;

};
}}}}

#endif // HTTP_SETCOOKIES_H
