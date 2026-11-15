#include "req_cookies.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocol::HTTP::Request;
using namespace Mantids30;

Cookies_ClientSide::Cookies_ClientSide() = default;

void Cookies_ClientSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    if (!m_cookiesMap.empty())
    {
        headers->add("Cookie", toString());
    }
}

std::string Cookies_ClientSide::toString() const
{
    std::vector<std::string> parts;
    parts.reserve(m_cookiesMap.size());

    for (const auto &i : m_cookiesMap)
    {
        parts.emplace_back(i.first + "=" + i.second);
    }

    return boost::algorithm::join(parts, "; ");
}

void Cookies_ClientSide::parseFromHeaders(std::string_view cookies_str)
{
    std::string localCookiesStr(cookies_str);
    vector<string> cookies;
    split(cookies, localCookiesStr, is_any_of(";"), token_compress_on);
    for (const string &cookie : cookies)
    {
        parseCookie(cookie);
    }
}

void Cookies_ClientSide::parseCookie(string cookie)
{
    size_t found = cookie.find('=');

    if (found != std::string::npos)
    {
        // We have parameters..
        std::string cookieValue = cookie.c_str() + found + 1;
        // TODO: check to use +1
        cookie.resize(found);

        trim(cookieValue);
        trim(cookie);

        addCookieVal(cookie, cookieValue);
    }
    else
    {
        // bad option!
    }
}

void Cookies_ClientSide::addCookieVal(const string &cookieName, const string &cookieValue)
{
    m_cookiesMap[cookieName] = cookieValue;
}

std::string Cookies_ClientSide::getCookieByName(std::string_view cookieName)
{
    const std::string name{cookieName};
    if (m_cookiesMap.find(name) == m_cookiesMap.end())
    {
        return "";
    }
    return m_cookiesMap[name];
}
