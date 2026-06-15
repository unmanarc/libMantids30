#include "api_options_handler.h"
#include <boost/algorithm/string/join.hpp>

using namespace Mantids30::API;

void OptionsHandlerConfig::configureAPIReturnOptionsHeaders(APIReturn &ret, const std::string & requestOrigin) const
{
    // Set Access-Control-Allow-Origin
    if (!requestOrigin.empty() && allowsOrigin(requestOrigin))
    {
        if (allowsAllOrigins())
        {
            ret.addHeader("Access-Control-Allow-Origin", "*");
        }
        else
        {
            ret.addHeader("Access-Control-Allow-Origin", requestOrigin);
        }
    }
    else if (!allowedOrigins.empty())
    {
        ret.addHeader("Access-Control-Allow-Origin", getOriginsHeader());
    }

    // Set Access-Control-Allow-Methods
    std::string methodsHeader = getMethodsHeader();
    if (!methodsHeader.empty())
    {
        ret.addHeader("Access-Control-Allow-Methods", methodsHeader);
    }
    else
    {
        // Default methods if none configured
        ret.addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    }

    // Set Access-Control-Allow-Headers
    std::string headersHeader = getHeadersHeader();
    if (!headersHeader.empty())
    {
        ret.addHeader("Access-Control-Allow-Headers", headersHeader);
    }
    else
    {
        // Default headers if none configured
        ret.addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }

    // Set Access-Control-Expose-Headers (only if configured)
    std::string exposeHeaders = getExposeHeadersHeader();
    if (!exposeHeaders.empty())
    {
        ret.addHeader("Access-Control-Expose-Headers", exposeHeaders);
    }

    // Set Access-Control-Allow-Credentials
    if (allowCredentials)
    {
        ret.addHeader("Access-Control-Allow-Credentials", "true");
    }

    // Set Access-Control-Max-Age
    if (maxAgeSeconds > 0)
    {
        ret.addHeader("Access-Control-Max-Age", std::to_string(maxAgeSeconds));
    }
}

OptionsHandlerConfig &Mantids30::API::OptionsHandlerConfig::insertAllowedOrigin(const std::string &allowedOrigin)
{
    allowedOrigins.insert(allowedOrigin);
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setAllowedOrigins(const std::set<std::string> &allowedOrigins)
{
    this->allowedOrigins = allowedOrigins;
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::insertAllowedMethod(const std::string &allowedMethod)
{
    allowedMethods.insert(allowedMethod);
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setAllowedMethods(const std::set<std::string> &allowedMethods)
{
    this->allowedMethods = allowedMethods;
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::insertAllowedHeader(const std::string &allowedHeader)
{
    allowedHeaders.insert(allowedHeader);
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setAllowedHeaders(const std::set<std::string> &allowedHeaders)
{
    this->allowedHeaders = allowedHeaders;
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::insertExposeHeader(const std::string &exposeHeader)
{
    exposeHeaders.insert(exposeHeader);
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setExposeHeaders(const std::set<std::string> &exposeHeaders)
{
    this->exposeHeaders = exposeHeaders;
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setAllowCredentials(bool allowCredentials)
{
    this->allowCredentials = allowCredentials;
    return *this;
}

OptionsHandlerConfig &OptionsHandlerConfig::setMaxAgeSeconds(int maxAgeSeconds)
{
    this->maxAgeSeconds = maxAgeSeconds;
    return *this;
}

bool OptionsHandlerConfig::allowsAllOrigins() const
{
    return allowedOrigins.count("*") > 0;
}

bool OptionsHandlerConfig::allowsOrigin(const std::string &origin) const
{
    if (allowsAllOrigins())
        return true;
    return allowedOrigins.count(origin) > 0;
}

std::string OptionsHandlerConfig::getOriginsHeader() const
{
    if (allowsAllOrigins())
        return "*";
    if (!allowedOrigins.empty())
        return *allowedOrigins.begin();
    return "";
}

std::string OptionsHandlerConfig::getMethodsHeader() const
{
    return boost::algorithm::join(allowedMethods, ", ");
}

std::string OptionsHandlerConfig::getHeadersHeader() const
{
    return boost::algorithm::join(allowedHeaders, ", ");
}

std::string OptionsHandlerConfig::getExposeHeadersHeader() const
{
    return boost::algorithm::join(exposeHeaders, ", ");
}
