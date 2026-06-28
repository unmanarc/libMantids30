#pragma once
/**
 * Provides utility functions for working with JSON data.
 */

#include <json/json.h>
#include <list>
#include <memory>
#include <set>

// Define a shorthand for the Json::Value type
using json = Json::Value;

namespace Mantids30::Helpers::JSON {

inline static const char *ASCSTRING(const Json::Value &j, const std::string &x, const char *def)
{
    if (j.isObject() && j.isMember(x) && j[x].isString())
    {
        return j[x].asCString();
    }
    return def;
}

inline static std::string ASSTRING(const Json::Value &j, const std::string &x, const std::string &def)
{
    if (j.isObject() && j.isMember(x) && j[x].isString())
    {
        return j[x].asString();
    }
    return def;
}

inline static bool ASBOOL(const Json::Value &j, const std::string &x, bool def)
{
    if (j.isObject() && j.isMember(x) && j[x].isBool())
    {
        return j[x].asBool();
    }
    return def;
}

inline static double ASDOUBLE(const Json::Value &j, const std::string &x, double def)
{
    if (j.isObject() && j.isMember(x) && j[x].isDouble())
    {
        return j[x].asDouble();
    }
    return def;
}

inline static float ASFLOAT(const Json::Value &j, const std::string &x, float def)
{
    if (j.isObject() && j.isMember(x) && j[x].isDouble())
    {
        return static_cast<float>(j[x].asFloat());
    }
    return def;
}

inline static int ASINT(const Json::Value &j, const std::string &x, int def)
{
    if (j.isObject() && j.isMember(x) && j[x].isInt())
    {
        return j[x].asInt();
    }
    return def;
}

inline static int64_t ASINT64(const Json::Value &j, const std::string &x, int64_t def)
{
    if (j.isObject() && j.isMember(x) && j[x].isInt64())
    {
        return j[x].asInt64();
    }
    return def;
}

inline static unsigned int ASUINT(const Json::Value &j, const std::string &x, unsigned int def)
{
    if (j.isObject() && j.isMember(x) && j[x].isUInt())
    {
        return j[x].asUInt();
    }
    return def;
}

inline static uint64_t ASUINT64(const Json::Value &j, const std::string &x, uint64_t def)
{
    if (j.isObject() && j.isMember(x) && j[x].isUInt64())
    {
        return j[x].asUInt64();
    }
    return def;
}

inline static bool ISARRAY(const Json::Value &j, const std::string &key)
{
    return j.isObject() && j.isMember(key) && j[key].isArray();
}

// --- Funciones para extraer valores de un valor JSON directo (sin clave) ---
// Los macros _D parecen asumir que 'j' ya es el valor específico, no el objeto padre.

inline static const char *ASCSTRING_D(const Json::Value &j, const char *def)
{
    return j.isString() ? j.asCString() : def;
}

inline static std::string ASSTRING_D(const Json::Value &j, const std::string &def)
{
    return j.isString() ? j.asString() : def;
}

inline static bool ASBOOL_D(const Json::Value &j, bool def)
{
    return j.isBool() ? j.asBool() : def;
}

inline static double ASDOUBLE_D(const Json::Value &j, double def)
{
    return j.isDouble() ? j.asDouble() : def;
}

inline static float ASFLOAT_D(const Json::Value &j, float def)
{
    return j.isDouble() ? static_cast<float>(j.asFloat()) : def;
}

inline static int ASINT_D(const Json::Value &j, int def)
{
    return j.isInt() ? j.asInt() : def;
}

inline static int64_t ASINT64_D(const Json::Value &j, int64_t def)
{
    return j.isInt64() ? j.asInt64() : def;
}

inline static unsigned int ASUINT_D(const Json::Value &j, unsigned int def)
{
    return j.isUInt() ? j.asUInt() : def;
}

inline static uint64_t ASUINT64_D(const Json::Value &j, uint64_t def)
{
    return j.isUInt64() ? j.asUInt64() : def;
}

inline static bool ISARRAY_D(const Json::Value &j)
{
    return j.isArray();
}

// --- Funciones para extraer valores de un array JSON por índice ---

inline static std::string ARRAY_ASSTRING(const Json::Value &j, size_t i, const std::string &def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isString())
    {
        return j[static_cast<uint32_t>(i)].asString();
    }
    return def;
}

inline static bool ARRAY_ASBOOL(const Json::Value &j, size_t i, bool def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isBool())
    {
        return j[static_cast<uint32_t>(i)].asBool();
    }
    return def;
}

inline static double ARRAY_ASDOUBLE(const Json::Value &j, size_t i, double def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isDouble())
    {
        return j[static_cast<uint32_t>(i)].asDouble();
    }
    return def;
}

inline static float ARRAY_ASFLOAT(const Json::Value &j, size_t i, float def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isDouble())
    {
        return static_cast<float>(j[static_cast<uint32_t>(i)].asFloat());
    }
    return def;
}

inline static int ARRAY_ASINT(const Json::Value &j, size_t i, int def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isInt())
    {
        return j[static_cast<uint32_t>(i)].asInt();
    }
    return def;
}

inline static int64_t ARRAY_ASINT64(const Json::Value &j, size_t i, int64_t def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isInt64())
    {
        return j[static_cast<uint32_t>(i)].asInt64();
    }
    return def;
}

inline static unsigned int ARRAY_ASUINT(const Json::Value &j, size_t i, unsigned int def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isUInt())
    {
        return j[static_cast<uint32_t>(i)].asUInt();
    }
    return def;
}

inline static uint64_t ARRAY_ASUINT64(const Json::Value &j, size_t i, uint64_t def)
{
    if (j.isArray() && i < j.size() && j[static_cast<uint32_t>(i)].isUInt64())
    {
        return j[static_cast<uint32_t>(i)].asUInt64();
    }
    return def;
}

Json::Value parse(const char *json);

std::map<std::string, std::string> toMap(const json &jValue);

/**
     * Converts a JSON value to a string.
     *
     * @param value The JSON value to convert.
     *
     * @return The JSON value as a string.
     */
std::string toString(const json &value);

/**
     * Converts a JSON array to a list of strings.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A list of strings containing the elements of the JSON array.
     */
std::list<std::string> toStringList(const json &value, const std::string &sub = "");

/**
     * Converts a JSON array to a set of strings.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A list of strings containing the elements of the JSON array.
     */
std::set<std::string> toStringSet(const json &value, const std::string &sub = "");
/**
     * Converts a JSON array to a set of uint32_t values.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A set of uint32_t containing the elements of the JSON array.
     */
std::set<uint32_t> toUInt32Set(const json &value, const std::string &sub = "");

/**
 * Converts a list of strings to a JSON array.
 *
 * @param t The list of strings to convert.
 *
 * @return A JSON array containing the elements of the input list.
 */
json fromList(const std::list<std::string> &t);

/**
     * Converts a set of unsigned 32-bit integers to a JSON array.
     *
     * @param t The set of uint32_t values to convert.
     *
     * @return A JSON array containing the elements of the input set.
     */
json fromSet(const std::set<uint32_t> &t);

/**
 * Converts a set of strings to a JSON array.
 *
 * This function takes a set of strings and converts it into a JSON array, where each
 * string in the set becomes an element in the resulting JSON array. The order of elements
 * in the JSON array is determined by the iteration order of the set.
 *
 * @param t The set of strings to convert.
 *
 * @return A JSON array containing the elements of the input set.
 */
json fromSet(const std::set<std::string> &t);

/**
 * A replacement for the deprecated Json::Reader class.
 *
 * Note: This class is named JSONReader2 to avoid conflicts with the existing JSONReader class in json.h.
 */
class JSONReader2
{
public:
    /**
     * Constructs a new JSON reader.
     */
    JSONReader2();

    /**
     * Parses a JSON document into a JSON value.
     *
     * @param document The JSON document to parse.
     * @param root A reference to a Json::Value object that will contain the parsed JSON value.
     *
     * @return True if the document was parsed successfully, false otherwise.
     */
    bool parse(const std::string &document, json &root);

    /**
     * Gets the error messages generated during the last parsing operation.
     *
     * @return A string containing the error messages.
     */
    std::string getFormattedErrorMessages();

private:
    std::shared_ptr<Json::CharReader> m_reader;
    std::string m_errors;
};

} // namespace Mantids30::Helpers::JSON
