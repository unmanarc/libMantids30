#include "rsp_cookies.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids::Protocols::HTTP::Response;
using namespace Mantids::Protocols::HTTP;
using namespace Mantids;

Cookies_ServerSide::Cookies_ServerSide()
{
}

Cookies_ServerSide::~Cookies_ServerSide()
{
    for (auto & cookie : cookiesMap) delete cookie.second;
}

void Cookies_ServerSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    for (const auto & cookie :cookiesMap )
    {
        headers->add("Set-Cookie",((Headers::Cookie *)cookie.second)->toSetCookieString(cookie.first));
    }
}

string Cookies_ServerSide::getCookieValueByName(const string &cookieName)
{
    Headers::Cookie * cookieValue = getCookieByName(cookieName);
    return !cookieValue?"":cookieValue->getValue();
}

Headers::Cookie * Cookies_ServerSide::getCookieByName(const string &cookieName)
{
    if (cookiesMap.find(cookieName) == cookiesMap.end()) return nullptr;
    return cookiesMap[cookieName];
}

bool Cookies_ServerSide::parseCookie(const string &cookie_str)
{
    std::string cookieName;
    Headers::Cookie * cookieValue = new Headers::Cookie;
    cookieValue->fromSetCookieString(cookie_str,&cookieName);
    if (cookieName.empty() || cookiesMap.find(cookieName) != cookiesMap.end())
    {
        delete cookieValue;
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

    Headers::Cookie * val = new Headers::Cookie;
    *val = cookieValue;

    cookiesMap[cookieName] = val;

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
        delete cookiesMap[cookieName];
        cookiesMap.erase(cookieName);
    }

    addCookieVal(cookieName,c);
}
