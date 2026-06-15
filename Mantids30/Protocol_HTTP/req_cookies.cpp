#include "req_cookies.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <utility>
#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP::Request;
using namespace Mantids30;

Cookies_ClientSide::Cookies_ClientSide()
{
}

void Cookies_ClientSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    if (!m_cookiesMap.empty())
        headers->add("Cookie",toString());
}

std::string Cookies_ClientSide::toString() const
{
    std::vector<std::string> parts;
    parts.reserve(m_cookiesMap.size());

    for (const std::pair<std::string,std::string>& i : m_cookiesMap)
    {
        parts.emplace_back(i.first + "=" + i.second);
    }

    return boost::algorithm::join(parts, "; ");
}

void Cookies_ClientSide::parseFromHeaders(const std::string &cookies_str)
{
    vector<string> cookies;
    split(cookies,cookies_str,is_any_of(";"),token_compress_on);
    for (const string & cookie : cookies) parseCookie(cookie);
}

void Cookies_ClientSide::parseCookie(string cookie)
{
    size_t found=cookie.find("=");

    if (found!=std::string::npos)
    {
        // We have parameters..
        std::string cookieValue = cookie.c_str()+found+1;
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

std::string Cookies_ClientSide::getCookieByName(const std::string &cookieName)
{
    if (m_cookiesMap.find(cookieName) == m_cookiesMap.end()) 
        return "";
    return m_cookiesMap[cookieName];
}
