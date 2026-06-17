#include "hdr_cookie.h"

#include <optional>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocol::HTTP::Headers;
using namespace Mantids30;

std::string Cookie::toSetCookieString(const std::string &cookieName) const
{
    std::string opts = cookieName + "=" + value + "; ";

    if (expires.has_value())
    {
        opts += "Expires=" + expires->toString() + "; ";
    }
    if (maxAge.has_value())
    {
        opts += "Max-Age=" + std::to_string(*maxAge) + "; ";
    }
    if (secure)
    {
        opts += "Secure; ";
    }
    if (httpOnly)
    {
        opts += "HttpOnly; ";
    }
    if (!domain.empty())
    {
        opts += "Domain=" + domain + "; ";
    }
    if (!path.empty())
    {
        opts += "Path=" + path + "; ";
    }

    switch (sameSitePolicy)
    {
    case SameSitePolicy::NONE:
        opts += "SameSite=None; ";
        break;
    case SameSitePolicy::STRICT:
        opts += "SameSite=Strict; ";
        break;
    case SameSitePolicy::LAX:
    default:
        opts += "SameSite=Lax; ";
        break;
    }

    return opts;
}

void Cookie::fromSetCookieString(const std::string &setCookieValue, string *cookieName)
{
    // Reset the current object to a clean state
    // This ensures that if parsing fails, the object doesn't hold stale data.
    *this = Cookie();

    vector<string> cookiesParams;
    split(cookiesParams, setCookieValue, is_any_of(";"), token_compress_on);

    bool firstVal = true;
    for (const string &param : cookiesParams)
    {
        std::pair<string, string> var = getVarNameAndValue(param);

        if (firstVal)
        {
            firstVal = false;
            // Get Value and
            *cookieName = var.first;
            value = var.second;
        }
        else
        {
            std::string varNameUpper = boost::to_upper_copy(var.first);
            if (varNameUpper == "EXPIRES")
            {
                expires.emplace();
                expires->fromString(var.second);
            }
            else if (varNameUpper == "MAX-AGE")
            {
                maxAge = strtoul(var.second.c_str(), nullptr, 10);
            }
            else if (varNameUpper == "SECURE")
            {
                secure = true;
            }
            else if (varNameUpper == "HTTPONLY")
            {
                httpOnly = true;
            }
            else if (varNameUpper == "DOMAIN")
            {
                domain = var.second;
            }
            else if (varNameUpper == "PATH")
            {
                path = var.second;
            }
            else if (varNameUpper == "SAMESITE")
            {
                if (boost::iequals(var.second, "LAX"))
                {
                    sameSitePolicy = SameSitePolicy::LAX;
                }
                else if (boost::iequals(var.second, "STRICT"))
                {
                    sameSitePolicy = SameSitePolicy::STRICT;
                }
                else
                {
                    sameSitePolicy = SameSitePolicy::NONE;
                }
            }
            else
            {
                // Unknown option...
            }
        }
    }
}

std::optional<time_t> Cookie::getExpiration() const
{
    if (expires.has_value())
    {
        return expires->getUnixTime();
    }
    return std::nullopt; // nullopt = indicating no expiration
}

void Cookie::setAsSessionCookie()
{
    // On session cookies there are no maxAge nor expires attributes...
    expires = std::nullopt;
    maxAge = std::nullopt;
}

void Cookie::setExpiration(const time_t &exp)
{
    expires.emplace();
    expires->setUnixTime(exp);
}

void Cookie::setExpirationFromNow(const uint32_t &seconds)
{
    expires.emplace();
    expires->setCurrentTime();
    expires->incTime(seconds);
}

void Cookie::deleteCookie()
{
    value = "";
    sameSitePolicy = SameSitePolicy::NONE;
    expires = std::nullopt;
    maxAge.emplace();
    maxAge = 0;
}

std::pair<string, string> Cookie::getVarNameAndValue(const string &var)
{
    std::pair<string, string> r;

    size_t found = var.find('=');

    if (found != std::string::npos)
    {
        // We have parameters..
        r.second = var.c_str() + found + 1;
        r.first = std::string(var.c_str(), found);
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
