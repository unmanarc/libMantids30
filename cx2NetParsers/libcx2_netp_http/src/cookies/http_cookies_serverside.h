#ifndef HTTP_SETCOOKIES_H
#define HTTP_SETCOOKIES_H

#include <string>
#include <map>

#include "http_cookie.h"

#include <cx2_netp_mime/mime_sub_header.h>

#include <stdint.h>
namespace CX2 { namespace Network { namespace HTTP {

class HTTP_Cookies_ServerSide
{
public:
    HTTP_Cookies_ServerSide();
    ~HTTP_Cookies_ServerSide();

    void putOnHeaders(MIME_Sub_Header * headers) const;

    std::string getCookieValueByName(const std::string & cookieName);
    HTTP_Cookie *getCookieByName(const std::string & cookieName);

    bool parseCookie(const std::string & cookie_str);
    bool addCookieVal(const std::string & cookieName, const HTTP_Cookie & cookieValue);

    /**
     * @brief addClearCookie Add cookie with empty values (to clear the previous cookie)
     * @param cookieName cookie Name
     */
    void addClearCookie(const std::string & cookieName);

private:
    std::map<std::string,HTTP_Cookie *> cookiesMap;

};
}}}

#endif // HTTP_SETCOOKIES_H
