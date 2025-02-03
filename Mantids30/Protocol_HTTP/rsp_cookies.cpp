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
    for (const auto & cookie :m_cookiesMap )
    {
        headers->add("Set-Cookie",(cookie.second)->toSetCookieString(cookie.first));
    }
}

string Cookies_ServerSide::getCookieValueByName(const string &cookieName)
{
    std::shared_ptr<Headers::Cookie> cookieValue = getCookieByName(cookieName);
    return !cookieValue?"":cookieValue->value;
}

std::shared_ptr<Headers::Cookie> Cookies_ServerSide::getCookieByName(const string &cookieName)
{
    if (m_cookiesMap.find(cookieName) == m_cookiesMap.end()) return nullptr;
    return m_cookiesMap[cookieName];
}

bool Cookies_ServerSide::parseCookie(const string &cookie_str)
{
    std::string cookieName;
    std::shared_ptr<Headers::Cookie> cookieValue = std::make_shared<Headers::Cookie>();
    cookieValue->fromSetCookieString(cookie_str,&cookieName);
    if (cookieName.empty() || m_cookiesMap.find(cookieName) != m_cookiesMap.end())
    {
        return false;
    }
    else
    {
        m_cookiesMap[cookieName] = cookieValue;
        return true;
    }
}

bool Cookies_ServerSide::addCookieVal(const string &cookieName, const Headers::Cookie &cookieValue)
{
    if (m_cookiesMap.find(cookieName) != m_cookiesMap.end()) return false;

    std::shared_ptr<Headers::Cookie> val = std::make_shared<Headers::Cookie>();
    *val = cookieValue;

    m_cookiesMap[cookieName] = val;

    return true;
}

bool Cookies_ServerSide::removeCookie(const std::string &cookieName)
{
    if (m_cookiesMap.find(cookieName) == m_cookiesMap.end()) return false;
    m_cookiesMap.erase(cookieName);
    return true;
}

void Cookies_ServerSide::addClearSecureCookie(const string &cookieName)
{
    Headers::Cookie c;

    c.value = "";
    c.secure = true;
    c.httpOnly = true;

    c.setAsTransientCookie();

    c.sameSitePolicy =  Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;

    if (m_cookiesMap.find(cookieName) != m_cookiesMap.end())
    {
        m_cookiesMap.erase(cookieName);
    }

    addCookieVal(cookieName,c);
}
