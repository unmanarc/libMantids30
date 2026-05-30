#include "api_options_handler.h"
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>

using namespace Mantids30::API::RESTful;


APIReturn buildCORSOptionsResponse(const OptionsHandlerConfig& config,
                                   const std::string& requestOrigin)
{
    APIReturn ret;
    ret.setStatus(Mantids30::Network::Protocols::HTTP::Status::S_204_NO_CONTENT);

    // Set Access-Control-Allow-Origin
    if (!requestOrigin.empty() && config.allowsOrigin(requestOrigin))
    {
        if (config.allowsAllOrigins())
        {
            ret.addHeader("Access-Control-Allow-Origin", "*");
        }
        else
        {
            ret.addHeader("Access-Control-Allow-Origin", requestOrigin);
        }
    }
    else if (!config.allowedOrigins.empty())
    {
        ret.addHeader("Access-Control-Allow-Origin", config.getOriginsHeader());
    }

    // Set Access-Control-Allow-Methods
    std::string methodsHeader = config.getMethodsHeader();
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
    std::string headersHeader = config.getHeadersHeader();
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
    std::string exposeHeaders = config.getExposeHeadersHeader();
    if (!exposeHeaders.empty())
    {
        ret.addHeader("Access-Control-Expose-Headers", exposeHeaders);
    }

    // Set Access-Control-Allow-Credentials
    if (config.allowCredentials)
    {
        ret.addHeader("Access-Control-Allow-Credentials", "true");
    }

    // Set Access-Control-Max-Age
    if (config.maxAgeSeconds > 0)
    {
        ret.addHeader("Access-Control-Max-Age", std::to_string(config.maxAgeSeconds));
    }

    return ret;
}