#pragma once

#include <string>
#include <Mantids29/Helpers/json.h>

namespace Mantids29 {
namespace Authentication {

/**
 * @brief The Data class represents the data associated with an authentication event.
 *
 * This class provides methods for serializing and deserializing authentication data, as well as getters
 * and setters for the password, password index, and domain name.
 */
class Data
{
public:
    /**
     * @brief Default constructor.
     */
    Data();

    /**
     * @brief Constructs an authentication data object with the given password, password index, and domain name.
     * @param password The password associated with the authentication event.
     * @param passwordIndex The index of the password in the password database.
     */
    Data(const std::string& password, const uint32_t& passwordIndex);

    /**
     * @brief Deserializes authentication data from a string.
     * @param serializedData The string containing the serialized authentication data.
     * @return true if the deserialization was successful, false otherwise.
     */
    bool setJsonString(const std::string& serializedData);

    /**
     * @brief Deserializes authentication data from a JSON object.
     * @param jsonObject The JSON object containing the authentication data.
     * @return true if the deserialization was successful, false otherwise.
     */
    bool setJson(const json& jsonObject);

    /**
     * @brief Serializes authentication data to a JSON object.
     * @return A JSON object containing the authentication data.
     */
    json toJson() const;


public:
    std::string m_password;
    uint32_t m_passwordIndex = 0;
};

}} // namespace Mantids29::Authentication


