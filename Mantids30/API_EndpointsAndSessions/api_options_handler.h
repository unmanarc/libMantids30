#pragma once

#include <string>
#include <set>
#include <vector>
#include <Mantids30/Helpers/json.h>

namespace Mantids30::API::RESTful {

// Forward declaration
class APIReturn;

/**
 * @brief Configuration for OPTIONS/CORS endpoint handling
 *
 * This struct holds CORS (Cross-Origin Resource Sharing) configuration
 * using natural C++ containers for intuitive setup and management.
 */
struct OptionsHandlerConfig
{
    // CORS standard settings using natural containers
    std::set<std::string> allowedOrigins;     ///< e.g., {"https://example.com", "https://app.com"} or {"*"}
    std::set<std::string> allowedMethods;     ///< e.g., {"GET", "POST", "PUT", "DELETE", "OPTIONS"}
    std::set<std::string> allowedHeaders;     ///< e.g., {"Content-Type", "Authorization"}
    std::set<std::string> exposeHeaders;      ///< e.g., {"X-Request-Id"}
    bool allowCredentials = false;            ///< Whether to allow credentials (cookies, auth headers)
    int maxAgeSeconds = 86400;                ///< Cache time in seconds for preflight requests

    /// Default constructor
    OptionsHandlerConfig() = default;

    /**
     * @brief Check if all origins are allowed (wildcard "*")
     * @return true if "*" is in allowedOrigins
     */
    bool allowsAllOrigins() const
    {
        return allowedOrigins.count("*") > 0;
    }

    /**
     * @brief Check if a specific origin is allowed
     * @param origin The origin to check
     * @return true if the origin is allowed
     */
    bool allowsOrigin(const std::string& origin) const
    {
        if (allowsAllOrigins())
            return true;
        return allowedOrigins.count(origin) > 0;
    }

    /**
     * @brief Get the Access-Control-Allow-Origin header value
     * @return "*" if all origins allowed, otherwise the first allowed origin or empty string
     */
    std::string getOriginsHeader() const
    {
        if (allowsAllOrigins())
            return "*";
        if (!allowedOrigins.empty())
            return *allowedOrigins.begin();
        return "";
    }

    /**
     * @brief Get the Access-Control-Allow-Methods header value (comma-separated)
     * @return Comma-separated string of allowed methods
     */
    std::string getMethodsHeader() const
    {
        std::string result;
        for (const auto& method : allowedMethods)
        {
            if (!result.empty())
                result += ", ";
            result += method;
        }
        return result;
    }

    /**
     * @brief Get the Access-Control-Allow-Headers header value (comma-separated)
     * @return Comma-separated string of allowed headers
     */
    std::string getHeadersHeader() const
    {
        std::string result;
        for (const auto& header : allowedHeaders)
        {
            if (!result.empty())
                result += ", ";
            result += header;
        }
        return result;
    }

    /**
     * @brief Get the Access-Control-Expose-Headers header value (comma-separated)
     * @return Comma-separated string of exposed headers
     */
    std::string getExposeHeadersHeader() const
    {
        std::string result;
        for (const auto& header : exposeHeaders)
        {
            if (!result.empty())
                result += ", ";
            result += header;
        }
        return result;
    }
};

/**
 * @brief Custom OPTIONS handler function type
 *
 * @param context User-defined context pointer
 * @param requestOrigin The Origin header from the request
 * @param config The OPTIONS/CORS configuration
 * @return APIReturn with the response
 */
using OptionsHandlerFunctionType = APIReturn (*)(void* context,
                                                 const std::string& requestOrigin,
                                                 const OptionsHandlerConfig& config);

/**
 * @brief Build a default CORS OPTIONS response from the configuration
 *
 * @param config The OPTIONS/CORS configuration
 * @param requestOrigin The Origin header from the request (optional)
 * @return APIReturn with CORS headers set
 */
APIReturn buildCORSOptionsResponse(const OptionsHandlerConfig& config,
                                   const std::string& requestOrigin = "");

} // namespace Mantids30::API::RESTful