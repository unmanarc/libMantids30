#include "hdr_cookie.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP::Headers;
using namespace Mantids30;

Cookie::Cookie()
{
    // Don't expire.
    expires.setUnixTime(0);
}

std::string Cookie::toSetCookieString(
    const std::string &cookieName) const
{
    std::string opts = cookieName + "=" + value + "; ";

    if (expires.getUnixTime()) opts+= "Expires=" + expires.toString() + "; ";
    if (maxAge.has_value()) opts+= "Max-Age=" + std::to_string(*maxAge) + "; ";
    if (secure) opts+= "Secure; ";
    if (httpOnly) opts+= "HttpOnly; ";
    if (!domain.empty()) opts+= "Domain=" + domain + "; ";
    if (!path.empty()) opts+= "Path=" + path + "; ";

    switch (sameSitePolicy) {
    case HTTP_COOKIE_SAMESITE_NONE:
        opts+= "SameSite=None; ";
        break;
    case HTTP_COOKIE_SAMESITE_STRICT:
        opts+= "SameSite=Strict; ";
        break;
    case HTTP_COOKIE_SAMESITE_LAX:
    default:
        opts+= "SameSite=Lax; ";
        break;
    }

    return opts;
}

void Cookie::fromSetCookieString(const std::string &setCookieValue, string *cookieName)
{
    vector<string> cookiesParams;
    split(cookiesParams,setCookieValue,is_any_of(";"),token_compress_on);

    *this = Cookie();

    bool firstVal = true;
    for (const string & param : cookiesParams)
    {
        std::pair<string, string> var = getVarNameAndValue(param);

        if (firstVal)
        {
            firstVal=false;
            // Get Value and
            *cookieName = var.first;
            value = var.second;
        }
        else
        {
            std::string varNameUpper = boost::to_upper_copy(var.first);

            if      (varNameUpper == "EXPIRES") expires.fromString(var.second);
            else if (varNameUpper == "MAX-AGE") maxAge = strtoul(var.second.c_str(),nullptr,10);
            else if (varNameUpper == "SECURE") secure = true;
            else if (varNameUpper == "HTTPONLY") httpOnly = true;
            else if (varNameUpper == "DOMAIN") domain = var.second;
            else if (varNameUpper == "PATH") path = var.second;
            else if (varNameUpper == "SAMESITE")
            {
                if ( boost::iequals( var.second, "LAX" ))
                    sameSitePolicy = HTTP_COOKIE_SAMESITE_LAX;
                else if ( boost::iequals( var.second, "STRICT" ))
                    sameSitePolicy = HTTP_COOKIE_SAMESITE_STRICT;
                else
                    sameSitePolicy = HTTP_COOKIE_SAMESITE_NONE;
            }
            else
            {
                // Unknown option...
            }
        }
    }
}

time_t Cookie::getExpiration() const
{
    return expires.getUnixTime();
}

void Cookie::setAsTransientCookie()
{
    setExpiration(1);
}

void Cookie::setExpiration(const time_t &exp)
{
    expires.setUnixTime(exp);
}

void Cookie::setExpirationFromNow(const uint32_t &seconds)
{
    expires.setCurrentTime();
    expires.incTime(seconds);
}

std::pair<string, string> Cookie::getVarNameAndValue(const string &var)
{
    std::pair<string, string> r;

    size_t found=var.find("=");

    if (found!=std::string::npos)
    {
        // We have parameters..
        r.second = var.c_str()+found+1;
        r.first = std::string(var.c_str(),found);
    }
    else
    {
        // Empty option...
        r.first = var;
    }

    trim(r.first);
    trim(r.second);

    return r;
}
