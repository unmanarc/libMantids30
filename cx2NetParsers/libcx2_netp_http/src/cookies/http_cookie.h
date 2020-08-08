#ifndef HTTP_COOKIE_VALUE_H
#define HTTP_COOKIE_VALUE_H

#include <string>
#include "http_date.h"

namespace CX2 { namespace Network { namespace HTTP {

enum eHTTP_Cookie_SameSitePolicy {
    HTTP_COOKIE_SAMESITE_NONE = 0,
    HTTP_COOKIE_SAMESITE_LAX = 1,
    HTTP_COOKIE_SAMESITE_STRICT = 2
};

class HTTP_Cookie
{
public:
    HTTP_Cookie();

    std::string toSetCookieString(const std::string & cookieName);
    bool fromSetCookieString(const std::string & setCookieValue, std::string * cookieName);

    ///////////////////////////////////////////////////////////////////////////////
    time_t getExpiration() const;
    void setExpiration(const time_t& exp);
    void setExpirationInSeconds(const uint32_t& seconds);

    std::string getValue() const;
    void setValue(const std::string &value);

    uint32_t getMaxAge() const;
    void setMaxAge(const uint32_t &value);

    std::string getDomain() const;
    void setDomain(const std::string &value);

    std::string getPath() const;
    void setPath(const std::string &value);

    bool isHttpOnly() const;
    void setHttpOnly(bool value);

    bool isSecure() const;
    void setSecure(bool value);


    eHTTP_Cookie_SameSitePolicy getSameSite() const;
    void setSameSite(const eHTTP_Cookie_SameSitePolicy &value);

private:
    std::pair<std::string, std::string> getVarNameAndValue(const std::string &var);

    std::string value;

    HTTP_Date expires;
    uint32_t max_age;
    std::string domain;
    std::string path;
    bool secure,httpOnly;
    eHTTP_Cookie_SameSitePolicy sameSite;
};

}}}

#endif // HTTP_COOKIE_VALUE_H
