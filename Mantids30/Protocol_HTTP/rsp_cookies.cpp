#include "rsp_cookies.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocol::HTTP::Response;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30;

void Cookies_ServerSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    for (const auto&cookie : m_cookiesMap)
    {
        headers->add("Set-Cookie", (cookie.second)->toSetCookieString(cookie.first));
    }
}

string Cookies_ServerSide::getCookieValueByName(const string &cookieName)
{
    std::shared_ptr<HTTP::Headers::Cookie> cookieValue = getCookieByName(cookieName);
    return !cookieValue ? "" : cookieValue->value;
}

std::shared_ptr<HTTP::Headers::Cookie> Cookies_ServerSide::getCookieByName(const string &cookieName)
{
    if (m_cookiesMap.find(cookieName) == m_cookiesMap.end())
    {
        return nullptr;
    }
    return m_cookiesMap[cookieName];
}

bool Cookies_ServerSide::parseCookie(const string &cookie_str)
{
    std::string cookieName;
    std::shared_ptr<Headers::Cookie> cookieValue = std::make_shared<Headers::Cookie>();
    cookieValue->fromSetCookieString(cookie_str, &cookieName);
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
    if (m_cookiesMap.find(cookieName) != m_cookiesMap.end())
    {
        return false;
    }

    std::shared_ptr<Headers::Cookie> val = std::make_shared<Headers::Cookie>();
    *val = cookieValue;

    m_cookiesMap[cookieName] = val;

    return true;
}

bool Cookies_ServerSide::removeCookie(const std::string &cookieName)
{
    if (m_cookiesMap.find(cookieName) == m_cookiesMap.end())
    {
        return false;
    }
    m_cookiesMap.erase(cookieName);
    return true;
}

void Cookies_ServerSide::addClearSecureCookie(const std::string &cookieName, const std::string &path)
{
    Headers::Cookie c;

    if (!path.empty())
    {
        c.path = path;
    }
    c.value = "";
    c.secure = true;
    c.httpOnly = true;

    c.deleteCookie();

    c.sameSitePolicy = Headers::Cookie::HTTP_COOKIE_SAMESITE_STRICT;

    if (m_cookiesMap.find(cookieName) != m_cookiesMap.end())
    {
        m_cookiesMap.erase(cookieName);
    }

    addCookieVal(cookieName, c);
}

void Cookies_ServerSide::addClearSecureCookie(const string &cookieName)
{
    addClearSecureCookie(cookieName, "");
}

void Cookies_ServerSide::prependPathToAllCookies(const std::string &prefix)
{
    if (prefix.empty())
    {
        return;
    }

    for (std::pair<const std::string, std::shared_ptr<HTTP::Headers::Cookie>> &cookie : m_cookiesMap)
    {
        // If cookie path is empty or "/", set it to the prefix directly
        if (cookie.second->path.empty() || cookie.second->path == "/")
        {
            cookie.second->path = prefix;
        }
        else
        {
            // Prepend prefix to existing path (e.g., /api -> /login/api)
            // Ensure we don't create double slashes
            std::string cleanPrefix = prefix;
            if (!cleanPrefix.empty() && cleanPrefix.back() == '/')
            {
                cleanPrefix.pop_back();
            }
            std::string cleanPath = cookie.second->path;
            if (!cleanPath.empty() && cleanPath.front() == '/')
            {
                // Path already starts with /, just concatenate
                cookie.second->path = cleanPrefix + cleanPath;
            }
            else
            {
                cookie.second->path = cleanPrefix + "/" + cleanPath;
            }
        }
    }
}
