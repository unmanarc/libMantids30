#pragma once

#include <string>
#include <stdint.h>
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP {

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
    Version() = default;

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
    std::string toString() const;

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

    /**
     * @brief Serializes the Version object into a JSON string.
     *
     * @return A JSON representation of the Version object.
     */
     std::string toJSON() const
     {
         Json::Value root;
         root["major"] = static_cast<int>(m_majorVersion);
         root["minor"] = static_cast<int>(m_minorVersion);
         return root.toStyledString();
     }
     /**
      * @brief Deserializes the Version object from a JSON string.
      *
      * @param jsonString The JSON string to parse and set the version numbers from.
      * @return True if parsing was successful, false otherwise.
      */
     bool fromJSON(const Json::Value & root)
     {
         m_majorVersion = static_cast<uint16_t>(JSON_ASUINT(root, "major", 0));
         m_minorVersion = static_cast<uint16_t>(JSON_ASUINT(root, "minor", 0));
         return true;
     }

private:
    uint16_t m_minorVersion = 1; ///< The minor version number.
    uint16_t m_majorVersion = 1; ///< The major version number.
};



}}}}

