#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <set>
#include <string>

namespace Mantids30::API {

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
    std::set<std::string> allowedOrigins; ///< e.g., {"https://example.com", "https://app.com"} or {"*"}
    std::set<std::string> allowedMethods; ///< e.g., {"GET", "POST", "PUT", "DELETE", "OPTIONS"}
    std::set<std::string> allowedHeaders; ///< e.g., {"Content-Type", "Authorization"}
    std::set<std::string> exposeHeaders;  ///< e.g., {"X-Request-Id"}
    bool allowCredentials = false;        ///< Whether to allow credentials (cookies, auth headers)
    int maxAgeSeconds = 86400;            ///< Cache time in seconds for preflight requests

    /// Default constructor
    OptionsHandlerConfig() = default;

    void configureAPIReturnOptionsHeaders(APIReturn &apiReturn, const std::string &requestOrigin) const;

    OptionsHandlerConfig &insertAllowedOrigin(const std::string &allowedOrigin);

    OptionsHandlerConfig &setAllowedOrigins(const std::set<std::string> &allowedOrigins);

    OptionsHandlerConfig &insertAllowedMethod(const std::string &allowedMethod);

    OptionsHandlerConfig &setAllowedMethods(const std::set<std::string> &allowedMethods);

    OptionsHandlerConfig &insertAllowedHeader(const std::string &allowedHeader);

    OptionsHandlerConfig &setAllowedHeaders(const std::set<std::string> &allowedHeaders);

    OptionsHandlerConfig &insertExposeHeader(const std::string &exposeHeader);

    OptionsHandlerConfig &setExposeHeaders(const std::set<std::string> &exposeHeaders);

    OptionsHandlerConfig &setAllowCredentials(bool allowCredentials);

    OptionsHandlerConfig &setMaxAgeSeconds(int maxAgeSeconds);

    /**
     * @brief Check if all origins are allowed (wildcard "*")
     * @return true if "*" is in allowedOrigins
     */
    bool allowsAllOrigins() const;

    /**
     * @brief Check if a specific origin is allowed
     * @param origin The origin to check
     * @return true if the origin is allowed
     */
    bool allowsOrigin(const std::string &origin) const;

    /**
     * @brief Get the Access-Control-Allow-Origin header value
     * @return "*" if all origins allowed, otherwise the first allowed origin or empty string
     */
    std::string getOriginsHeader() const;

    /**
     * @brief Get the Access-Control-Allow-Methods header value (comma-separated)
     * @return Comma-separated string of allowed methods
     */
    std::string getMethodsHeader() const;

    /**
     * @brief Get the Access-Control-Allow-Headers header value (comma-separated)
     * @return Comma-separated string of allowed headers
     */
    std::string getHeadersHeader() const;

    /**
     * @brief Get the Access-Control-Expose-Headers header value (comma-separated)
     * @return Comma-separated string of exposed headers
     */
    std::string getExposeHeadersHeader() const;
};

} // namespace Mantids30::API
