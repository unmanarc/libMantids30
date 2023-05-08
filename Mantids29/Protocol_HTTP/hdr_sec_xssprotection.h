#pragma once

#include <string>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Headers { namespace Security {

class XSSProtection
{
public:
    /**
     * @brief Constructs a new XSSProtection object with default values.
     */
    XSSProtection();
    /**
     * @brief Constructs a new XSSProtection object with the given options.
     *
     * @param activated Whether the X-XSS-Protection header is activated.
     * @param blocking Whether to enable blocking for the X-XSS-Protection header.
     */
    XSSProtection(bool activated, bool blocking);
    /**
     * @brief Constructs a new XSSProtection object with the given options.
     *
     * @param activated Whether the X-XSS-Protection header is activated.
     * @param reportURL The URL to report violations to for the X-XSS-Protection header.
     */
    XSSProtection(bool activated, const std::string &reportURL);
    /**
     * @brief Returns the X-XSS-Protection header as a string value.
     *
     * @return The X-XSS-Protection header as a string value.
     */
    std::string toString();
    /**
     * @brief Parses the given X-XSS-Protection header string and sets the options accordingly.
     *
     * @param sValue The X-XSS-Protection header string to parse.
     * @return true if the parsing was successful, false otherwise.
     */
    bool fromString(const std::string & sValue);
    /**
     * @brief Sets the X-XSS-Protection header to its default values.
     */
    void setDefaults();

    bool m_parameterActivated; ///< Whether the X-XSS-Protection header is activated.
    bool m_modeBlocking; ///< Whether blocking is enabled for the X-XSS-Protection header.
    std::string m_reportURL; ///< The URL to report violations to for the X-XSS-Protection header.
};

}}}}}}



