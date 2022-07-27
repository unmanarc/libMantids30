#include "hdr_cookie.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids::Protocols::HTTP::Headers;
using namespace Mantids;

Cookie::Cookie()
{
    setDefaults();
}

void Cookie::setDefaults()
{
    // Don't expire.
    expires.setRawTime(0);
    max_age=std::numeric_limits<uint32_t>::max();
    secure=false;
    httpOnly=false;
    sameSite = HTTP_COOKIE_SAMESITE_LAX;

    value="";
    domain="";
    path="";
}

std::string Cookie::toSetCookieString(const std::string &cookieName)
{
    std::string opts = cookieName + "=" + value + "; ";

    if (expires.getRawTime()) opts+= "Expires=" + expires.toString() + "; ";
    if (max_age!=std::numeric_limits<uint32_t>::max()) opts+= "Max-Age=" + std::to_string(max_age) + "; ";
    if (secure) opts+= "Secure; ";
    if (httpOnly) opts+= "HttpOnly; ";
    if (!domain.empty()) opts+= "Domain=" + domain + "; ";
    if (!path.empty()) opts+= "Path=" + path + "; ";

    switch (sameSite) {
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

    setDefaults();

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
            else if (varNameUpper == "MAX-AGE") max_age = strtoul(var.second.c_str(),nullptr,10);
            else if (varNameUpper == "SECURE") secure = true;
            else if (varNameUpper == "HTTPONLY") httpOnly = true;
            else if (varNameUpper == "DOMAIN") domain = var.second;
            else if (varNameUpper == "PATH") path = var.second;
            else if (varNameUpper == "SAMESITE")
            {
                if ( boost::iequals( var.second, "LAX" ))
                    sameSite = HTTP_COOKIE_SAMESITE_LAX;
                else if ( boost::iequals( var.second, "STRICT" ))
                    sameSite = HTTP_COOKIE_SAMESITE_STRICT;
                else
                    sameSite = HTTP_COOKIE_SAMESITE_NONE;
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
    return expires.getRawTime();
}

void Cookie::setAsTransientCookie()
{
    setExpiration(0);
}

void Cookie::setExpiration(const time_t &exp)
{
    expires.setRawTime(exp);
}

void Cookie::setExpirationFromNow(const uint32_t &seconds)
{
    expires.setCurrentTime();
    expires.incTime(seconds);
}

std::string Cookie::getValue() const
{
    return value;
}

void Cookie::setValue(const std::string &value)
{
    this->value = value;
}

uint32_t Cookie::getMaxAge() const
{
    return max_age;
}

void Cookie::setMaxAge(const uint32_t &value)
{
    max_age = value;
}

std::string Cookie::getDomain() const
{
    return domain;
}

void Cookie::setDomain(const std::string &value)
{
    domain = value;
}

std::string Cookie::getPath() const
{
    return path;
}

void Cookie::setPath(const std::string &value)
{
    path = value;
}

bool Cookie::isHttpOnly() const
{
    return httpOnly;
}

void Cookie::setHttpOnly(bool value)
{
    httpOnly = value;
}

bool Cookie::isSecure() const
{
    return secure;
}

void Cookie::setSecure(bool value)
{
    secure = value;
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

Cookie::eSameSitePolicy Cookie::getSameSite() const
{
    return sameSite;
}

void Cookie::setSameSite(const Cookie::eSameSitePolicy &value)
{
    sameSite = value;
}
