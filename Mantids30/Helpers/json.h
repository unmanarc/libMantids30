#pragma once
/**
 * Provides utility functions for working with JSON data.
 */

#include <json/json.h>
#include <list>
#include <set>

// Define a shorthand for the Json::Value type
typedef Json::Value json;

// jsoncpp macros:
#define JSON_ASCSTRING(j, x, def) \
((j).isObject() && (j).isMember(x) && (j)[x].isString() ? (j)[x].asCString() : (def))

#define JSON_ASSTRING(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isString() ? (j)[x].asString() : (def))

#define JSON_ASBOOL(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isBool() ? (j)[x].asBool() : (def))

#define JSON_ASDOUBLE(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isDouble() ? (j)[x].asDouble() : (def))

#define JSON_ASFLOAT(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isFloat() ? (j)[x].asFloat() : (def))

#define JSON_ASINT(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isInt() ? (j)[x].asInt() : (def))

#define JSON_ASINT64(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isInt64() ? (j)[x].asInt64() : (def))

#define JSON_ASUINT(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isUInt() ? (j)[x].asUInt() : (def))

#define JSON_ASUINT64(j, x, def) \
    ((j).isObject() && (j).isMember(x) && (j)[x].isUInt64() ? (j)[x].asUInt64() : (def))

#define JSON_ISARRAY(j, key) \
    ((j).isObject() && (j).isMember(key) && (j)[(key)].isArray())

#define JSON_ASCSTRING_D(j, def) \
    (j.isString()?j.asCString():def)

#define JSON_ASSTRING_D(j, def) \
    (j.isString()?j.asString():def)

#define JSON_ASBOOL_D(j, def) \
    (j.isBool()?j.asBool():def)

#define JSON_ASDOUBLE_D(j, def)  \
    (j.isDouble()?j.asDouble():def)

#define JSON_ASFLOAT_D(j, def) \
    (j.isFloat()?j.asFloat():def)

#define JSON_ASINT_D(j, def) \
    (j.isInt()?j.asInt():def)

#define JSON_ASINT64_D(j, def) \
    (j.isInt64()?j.asInt64():def)

#define JSON_ASUINT_D(j, def) \
    (j.isUInt()?j.asUInt():def)

#define JSON_ASUINT64_D(j, def) \
    (j.isUInt64()?j.asUInt64():def)

#define JSON_ISARRAY_D(j)  \
    (j.isArray())

#define JSON_ARRAY_ASSTRING(j, i, def) \
((j).isArray() && ((i) < (j).size()) && (j)[(i)].isString() ? (j)[(i)].asString() : (def))

#define JSON_ARRAY_ASBOOL(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isBool() ? (j)[(i)].asBool() : (def))

#define JSON_ARRAY_ASDOUBLE(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isDouble() ? (j)[(i)].asDouble() : (def))

#define JSON_ARRAY_ASFLOAT(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isFloat() ? (j)[(i)].asFloat() : (def))

#define JSON_ARRAY_ASINT(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isInt() ? (j)[(i)].asInt() : (def))

#define JSON_ARRAY_ASINT64(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isInt64() ? (j)[(i)].asInt64() : (def))

#define JSON_ARRAY_ASUINT(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isUInt() ? (j)[(i)].asUInt() : (def))

#define JSON_ARRAY_ASUINT64(j, i, def) \
    ((j).isArray() && ((i) < (j).size()) && (j)[(i)].isUInt64() ? (j)[(i)].asUInt64() : (def))

namespace Mantids30 {
namespace Helpers {

    /**
     * Converts a JSON value to a string.
     *
     * @param value The JSON value to convert.
     *
     * @return The JSON value as a string.
     */
    std::string jsonToString(const json &value);

    /**
     * Converts a JSON array to a list of strings.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A list of strings containing the elements of the JSON array.
     */
    std::list<std::string> jsonToStringList(const json &value, const std::string &sub = "");

    /**
     * Converts a JSON array to a set of strings.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A list of strings containing the elements of the JSON array.
     */
    std::set<std::string> jsonToStringSet(const json &value, const std::string &sub = "");
    /**
     * Converts a JSON array to a set of uint32_t values.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A set of uint32_t containing the elements of the JSON array.
     */
    std::set<uint32_t> jsonToUInt32Set(const json &value, const std::string &sub = "");

    /**
     * Converts a set of strings to a JSON array.
     *
     * @param t The set of strings to convert.
     *
     * @return A JSON array containing the elements of the input set.
     */
    json setToJSON(const std::set<std::string> &t);

    /**
     * Converts a list of strings to a JSON array.
     *
     * @param t The list of strings to convert.
     *
     * @return A JSON array containing the elements of the input list.
     */
    json listToJSON(const std::list<std::string> &t);

    /**
     * Converts a set of unsigned 32-bit integers to a JSON array.
     *
     * @param t The set of uint32_t values to convert.
     *
     * @return A JSON array containing the elements of the input set.
     */
    json setToJSON(const std::set<uint32_t> &t);

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
    json setToJSON(const std::set<std::string> &t);

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

} // namespace Helpers
} // namespace Mantids30
