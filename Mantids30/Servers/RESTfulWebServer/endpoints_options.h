#pragma once

#include "api_options_handler.h"

namespace Mantids30::API {

class Endpoints_Options
{
public:
    Endpoints_Options() = default;

    /**
     * @brief Enable and configure OPTIONS/CORS handling for all endpoints
     * or for a specific endpoint.
     *
     * @param endpointPath Optional: if empty, applies to ALL endpoints (global).
     *                     If set, applies only to that specific endpoint.
     * @param config The CORS configuration to apply
     */
    void setEndpointOptions(const std::string &endpointPath, const OptionsHandlerConfig &config);

    /**
     * @brief Enable and configure OPTIONS/CORS handling for all endpoints
     *
     * @param config The CORS configuration to apply
     */
    void setEndpointOptions(const OptionsHandlerConfig &config) { setEndpointOptions("", config); }

    /**
     * @brief Get the global CORS configuration
     * @return Reference to the global OPTIONS/CORS config
     */
    [[nodiscard]] const OptionsHandlerConfig *getGlobalOptionsConfig() const;

    [[nodiscard]] const OptionsHandlerConfig *getOptionsConfigOnEndpoint(const std::string &endpointPath) const;

    /**
     * @brief Check if OPTIONS/CORS handling is enabled
     * @return true if CORS is enabled
     */
    [[nodiscard]] bool isOptionsEnabled() const { return m_optionsEnabled; }
    /**
 * @brief Build a default CORS OPTIONS response from the configuration
 *
 * @param config The OPTIONS/CORS configuration
 * @param requestOrigin The Origin header from the request (optional)
 * @return APIReturn with CORS headers set
 */
    [[nodiscard]] APIReturn buildCORSOptionsResponse(const OptionsHandlerConfig &config, const std::string &requestOrigin = "");

private:
    OptionsHandlerConfig m_globalOptionsConfig;                             ///< Global CORS/OPTIONS configuration
    std::map<std::string, OptionsHandlerConfig> m_perEndpointOptionsConfig; ///< Per-endpoint CORS configuration
    bool m_optionsEnabled = false;                                          ///< Whether OPTIONS/CORS handling is enabled
};

} // namespace Mantids30::API
