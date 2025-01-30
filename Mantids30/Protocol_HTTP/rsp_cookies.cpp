#include "rsp_cookies.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP::Response;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30;

Cookies_ServerSide::Cookies_ServerSide()
{
}

void Cookies_ServerSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    for (const auto & cookie :cookiesMap )
    {
        headers->add("Set-Cookie",(cookie.second)->toSetCookieString(cookie.first));
    }
}

string Cookies_ServerSide::getCookieValueByName(const string &cookieName)
{
    std::shared_ptr<Headers::Cookie> cookieValue = getCookieByName(cookieName);
    return !cookieValue?"":cookieValue->getValue();
}

std::shared_ptr<Headers::Cookie> Cookies_ServerSide::getCookieByName(const string &cookieName)
{
    if (cookiesMap.find(cookieName) == cookiesMap.end()) return nullptr;
    return cookiesMap[cookieName];
}

bool Cookies_ServerSide::parseCookie(const string &cookie_str)
{
    std::string cookieName;
    std::shared_ptr<Headers::Cookie> cookieValue = std::make_shared<Headers::Cookie>();
    cookieValue->fromSetCookieString(cookie_str,&cookieName);
    if (cookieName.empty() || cookiesMap.find(cookieName) != cookiesMap.end())
    {
        return false;
    }
    else
    {
        cookiesMap[cookieName] = cookieValue;
        return true;
    }
}

bool Cookies_ServerSide::addCookieVal(const string &cookieName, const Headers::Cookie &cookieValue)
{
    if (cookiesMap.find(cookieName) != cookiesMap.end()) return false;

    std::shared_ptr<Headers::Cookie> val = std::make_shared<Headers::Cookie>();
    *val = cookieValue;

    cookiesMap[cookieName] = val;

    return true;
}

bool Cookies_ServerSide::removeCookie(const std::string &cookieName)
{
    if (cookiesMap.find(cookieName) == cookiesMap.end()) return false;
    cookiesMap.erase(cookieName);
    return true;
}

void Cookies_ServerSide::addClearSecureCookie(const string &cookieName)
{
    Headers::Cookie c;

    c.setValue("");
    c.setSecure(true);
    c.setHttpOnly(true);
    c.setAsTransientCookie();
    c.setSameSite(Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT);

    if (cookiesMap.find(cookieName) != cookiesMap.end())
    {
        cookiesMap.erase(cookieName);
    }

    addCookieVal(cookieName,c);
}
