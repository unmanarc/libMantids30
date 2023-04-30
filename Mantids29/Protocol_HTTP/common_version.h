#ifndef HTTP_VERSION_H
#define HTTP_VERSION_H

#include <string>
#include <stdint.h>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

/**
 * @brief The Version class provides functionality for HTTP versions.
 *
 * The class provides functions to parse a version string, get the version as a string, get and set the major and minor version numbers, and upgrade the minor version number.
 */
class Version
{
public:
    /**
     * @brief Constructs a new Version object with version 1.0.
     */
    Version();

    /**
     * @brief Parses the given version string into the major and minor version numbers.
     *
     * @param version The version string to parse.
     */
    void parse(const std::string& version);

    /**
     * @brief Returns the HTTP version as a string.
     *
     * @return The HTTP version as a string.
     */
    std::string toString();

    /**
     * @brief Returns the minor version number.
     *
     * @return The minor version number.
     */
    uint16_t getMinor() const;

    /**
     * @brief Sets the minor version number to the given value.
     *
     * @param value The value to set the minor version number to.
     */
    void setMinor(const uint16_t& value);

    /**
     * @brief Upgrades the minor version number by the given value.
     *
     * @param value The value to increment the minor version number by.
     */
    void upgradeMinor(const uint16_t& value);

    /**
     * @brief Returns the major version number.
     *
     * @return The major version number.
     */
    uint16_t getMajor() const;

    /**
     * @brief Sets the major version number to the given value.
     *
     * @param value The value to set the major version number to.
     */
    void setMajor(const uint16_t& value);

private:
    uint16_t m_minorVersion = 1; ///< The minor version number.
    uint16_t m_majorVersion = 1; ///< The major version number.
};



}}}}}

#endif // HTTP_VERSION_H
