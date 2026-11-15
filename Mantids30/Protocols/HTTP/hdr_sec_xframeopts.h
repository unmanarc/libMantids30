#pragma once

#include <cstdint>
#include <string>

namespace Mantids30::Network::Protocol::HTTP::Headers::Security {

/**
 * @brief The XFrameOptions class provides functionality for the X-Frame-Options header.
 *
 * The class provides functions to set and get the X-Frame-Options header value, including options for "none", "deny", "sameorigin", and "allow-from".
 */
class XFrameOptions
{
public:
    /**
     * @brief Enumeration of the possible values for the X-Frame-Options header.
     */
    enum class Option : uint8_t
    {
        NONE = 0,       ///< X-Frame-Options value of "none".
        DENY = 1,       ///< X-Frame-Options value of "deny".
        SAMEORIGIN = 2, ///< X-Frame-Options value of "sameorigin".
        ALLOWFROM = 3   ///< X-Frame-Options value of "allow-from".
    };

    /**
     * @brief Constructs a new XFrameOptions object with default values.
     */
    XFrameOptions() = default;

    /**
     * @brief Constructs a new XFrameOptions object with the given options.
     *
     * @param value The X-Frame-Options value.
     * @param fromURL The URL to allow frames from when the value is "allow-from".
     */
    XFrameOptions(const Option &value, const std::string &allowFromURL = "");

    /**
     * @brief Returns whether the X-Frame-Options header is not activated.
     *
     * @return true if the X-Frame-Options header is not activated, false otherwise.
     */
    [[nodiscard]] bool isNotActivated() const;

    /**
     * @brief Returns the X-Frame-Options header as a string value.
     *
     * @return The X-Frame-Options header as a string value.
     */
    [[nodiscard]] std::string toString() const;

    /**
     * @brief Parses the given X-Frame-Options header string and sets the options accordingly.
     *
     * @param sValue The X-Frame-Options header string to parse.
     * @return true if the parsing was successful, false otherwise.
     */
    bool fromString(const std::string &sValue);

    std::string allowFromURL;    ///< The URL to allow frames from when the value is "allow-from".
    Option value = Option::DENY; ///< The X-Frame-Options value.
};

} // namespace Mantids30::Network::Protocol::HTTP::Headers::Security
