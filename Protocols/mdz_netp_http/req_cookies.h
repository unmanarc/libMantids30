#ifndef HTTP_COOKIES_H
#define HTTP_COOKIES_H

#include <string>
#include <map>

#include <mdz_netp_mime/mime_sub_header.h>

namespace Mantids { namespace Network { namespace HTTP { namespace Request {

class Cookies_ClientSide
{
public:
    Cookies_ClientSide();

    void putOnHeaders(MIME::MIME_Sub_Header * headers) const;
    /**
     * @brief parseFromHeaders Parse cookies from string
     * @param cookies_str string to parse
     */
    void parseFromHeaders(const std::string & cookies_str);
    ///////////////////////////

    std::string getCookieByName(const std::string & cookieName);
    void addCookieVal(const std::string & cookieName, const std::string & cookieValue);

    // TODO: add from server cookies (for client use)

private:
    /**
     * @brief toString Convert the cookie to string to send on headers.
     * @return string with cookies.
     */
    std::string toString() const;

    void parseCookie(std::string cookie);
    std::map<std::string,std::string> cookiesMap;
};

}}}}

#endif // HTTP_COOKIES_H
