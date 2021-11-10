#include "req_cookies.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP::Request;
using namespace CX2;

Cookies_ClientSide::Cookies_ClientSide()
{
}

void Cookies_ClientSide::putOnHeaders(MIME::MIME_Sub_Header *headers) const
{
    headers->add("Cookie",toString());
}

std::string Cookies_ClientSide::toString() const
{
    std::string cookies;

    bool first = true;
    for (auto & i : cookiesMap)
    {
        if (first)
        {
            cookies+="; ";
            first = false;
        }
        cookies+=i.first;
        cookies+="=";
        cookies+=i.second;
    }
    return cookies;
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
    cookiesMap[cookieName] = cookieValue;
}

std::string Cookies_ClientSide::getCookieByName(const std::string &cookieName)
{
    if (cookiesMap.find(cookieName) == cookiesMap.end()) return "";
    return cookiesMap[cookieName];
}
