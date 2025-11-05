#pragma once

#include <string>
#include <stdint.h>

namespace Mantids30::Network::Protocols::HTTP { namespace Headers { namespace Security {

/**
 * @brief The HSTS class provides functionality for HTTP Strict Transport Security (HSTS).
 *
 * The class provides functions to set and get the HSTS options, including the maximum age, whether to include subdomains, and whether to use the preload option.
 */
class HSTS
{
public:
    /**
     * @brief Constructs a new HSTS object with default values.
     */
    HSTS();

    /**
     * @brief Constructs a new HSTS object with the given options.
     *
     * @param maxAge The maximum age in seconds for which the HSTS policy is enforced.
     * @param includeSubdomains Whether to include subdomains in the HSTS policy.
     * @param preload Whether to use the preload option, which includes the domain in the preload list.
     */
    HSTS(uint32_t maxAge, bool includeSubdomains = false, bool preload = false);

    /**
     * @brief Sets the HSTS options to their default values.
     */
    void setDefaults();

    /**
     * @brief Returns the HSTS policy as a string value.
     *
     * @return The HSTS policy as a string value.
     */
    std::string toString();

    /**
     * @brief Parses the given HSTS policy string and sets the options accordingly.
     *
     * @param sValue The HSTS policy string to parse.
     * @return true if the parsing was successful, false otherwise.
     */
    bool fromString(const std::string& sValue);

    bool isActivated = false; ///< Whether the HSTS policy is activated.
    bool isPreloadEnabled = false; ///< Whether to use the preload option.
    bool isSubdomainIncluded = false; ///< Whether to include subdomains in the HSTS policy.
private:
    uint32_t m_maxAge = 0; ///< The maximum age in seconds for which the HSTS policy is enforced.
};


}}}

