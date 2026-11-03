#pragma once

/**
 * JSON conversion functions and Reader class.
 */

#include <boost/json.hpp>
#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <string>

namespace Mantids30::Helpers::JSON {

boost::json::value parse(const char *json);

std::map<std::string, std::string> toMap(const boost::json::value &jValue);

/**
 * Converts a JSON value to a string.
 *
 * @param value The JSON value to convert.
 *
 * @return The JSON value as a string.
 */
std::string toString(const boost::json::value &value);

/**
 * Converts a JSON array to a list of strings.
 *
 * @param value The JSON array to convert.
 * @param sub The name of the sub-element to extract from each array element (optional).
 *
 * @return A list of strings containing the elements of the JSON array.
 */
std::list<std::string> toStringList(const boost::json::value &value, const std::string &sub = "");

/**
 * Converts a JSON array to a set of strings.
 *
 * @param value The JSON array to convert.
 * @param sub The name of the sub-element to extract from each array element (optional).
 *
 * @return A set of strings containing the elements of the JSON array.
 */
std::set<std::string> toStringSet(const boost::json::value &value, const std::string &sub = "");

/**
 * Converts a JSON array to a set of uint32_t values.
 *
 * @param value The JSON array to convert.
 * @param sub The name of the sub-element to extract from each array element (optional).
 *
 * @return A set of uint32_t containing the elements of the JSON array.
 */
std::set<uint32_t> toUInt32Set(const boost::json::value &value, const std::string &sub = "");

/**
 * Converts a list of strings to a JSON array.
 *
 * @param t The list of strings to convert.
 *
 * @return A JSON array containing the elements of the input list.
 */
boost::json::value fromList(const std::list<std::string> &t);

/**
 * Converts a set of unsigned 32-bit integers to a JSON array.
 *
 * @param t The set of uint32_t values to convert.
 *
 * @return A JSON array containing the elements of the input set.
 */
boost::json::value fromSet(const std::set<uint32_t> &t);

/**
 * Converts a set of strings to a JSON array.
 *
 * @param t The set of strings to convert.
 *
 * @return A JSON array containing the elements of the input set.
 */
boost::json::value fromSet(const std::set<std::string> &t);

/**
 * Parses a JSON document into a boost::json value.
 *
 * @param document The JSON document to parse.
 * @param root A reference to a boost::json::value that will contain the parsed JSON value.
 * @param parseErrors Optional pointer to a std::string that receives the error message
 *        when parsing fails. Kept out-of-band (thread-safe: no shared state).
 *
 * @return True if the document was parsed successfully, false otherwise.
 */
bool parse(const std::string &document, boost::json::value &root, std::string *parseErrors = nullptr);

/**
 * A convenience wrapper around boost::json::parse that matches the previous
 * JSONReader interface.
 */
class Reader
{
public:
    Reader() = default;

    /**
     * Parses a JSON document into a JSON value.
     *
     * @param document The JSON document to parse.
     * @param root A reference to a boost::json::value that will contain the parsed JSON value.
     *
     * @return True if the document was parsed successfully, false otherwise.
     */
    bool parse(const std::string &document, boost::json::value &root);

    /**
     * Gets the error messages generated during the last parsing operation.
     *
     * @return A string containing the error messages.
     */
    std::string getFormattedErrorMessages();

private:
    std::string m_errors;
};

} // namespace Mantids30::Helpers::JSON
