#include "http_cookie.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2;

HTTP_Cookie::HTTP_Cookie()
{
    // Don't expire.
    expires.setRawTime(0);
    max_age=std::numeric_limits<uint32_t>::max();
    secure=false;
    httpOnly=false;
    sameSite = HTTP_COOKIE_SAMESITE_LAX;
}

std::string HTTP_Cookie::toSetCookieString(const std::string &cookieName)
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

bool HTTP_Cookie::fromSetCookieString(const std::string &setCookieValue, string *cookieName)
{
    vector<string> cookiesParams;
    split(cookiesParams,setCookieValue,is_any_of(";"),token_compress_on);

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

    return true;
}

time_t HTTP_Cookie::getExpiration() const
{
    return expires.getRawTime();
}

void HTTP_Cookie::setToExpire()
{
    setExpiration(0);
}

void HTTP_Cookie::setExpiration(const time_t &exp)
{
    expires.setRawTime(exp);
}

void HTTP_Cookie::setExpirationInSeconds(const uint32_t &seconds)
{
    expires.setCurrentTime();
    expires.incTime(seconds);
}

std::string HTTP_Cookie::getValue() const
{
    return value;
}

void HTTP_Cookie::setValue(const std::string &value)
{
    this->value = value;
}

uint32_t HTTP_Cookie::getMaxAge() const
{
    return max_age;
}

void HTTP_Cookie::setMaxAge(const uint32_t &value)
{
    max_age = value;
}

std::string HTTP_Cookie::getDomain() const
{
    return domain;
}

void HTTP_Cookie::setDomain(const std::string &value)
{
    domain = value;
}

std::string HTTP_Cookie::getPath() const
{
    return path;
}

void HTTP_Cookie::setPath(const std::string &value)
{
    path = value;
}

bool HTTP_Cookie::isHttpOnly() const
{
    return httpOnly;
}

void HTTP_Cookie::setHttpOnly(bool value)
{
    httpOnly = value;
}

bool HTTP_Cookie::isSecure() const
{
    return secure;
}

void HTTP_Cookie::setSecure(bool value)
{
    secure = value;
}

std::pair<string, string> HTTP_Cookie::getVarNameAndValue(const string &var)
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

eHTTP_Cookie_SameSitePolicy HTTP_Cookie::getSameSite() const
{
    return sameSite;
}

void HTTP_Cookie::setSameSite(const eHTTP_Cookie_SameSitePolicy &value)
{
    sameSite = value;
}
