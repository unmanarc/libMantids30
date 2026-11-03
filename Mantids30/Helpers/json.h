#pragma once

/**
 * Provides utility functions for working with JSON data using Boost.JSON.
 */

#include <boost/json.hpp>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace Mantids30::Helpers::JSON {

namespace detail {

// Returns the member value of an object, or a null value if the object does
// not contain the key (mirrors jsoncpp's non-throwing operator[] behavior).
inline const boost::json::value &safeGet(const boost::json::value &obj, const std::string &key)
{
    if (obj.is_object())
    {
        if (auto it = obj.as_object().find(key); it != obj.as_object().end())
        {
            return it->value();
        }
    }
    static const boost::json::value nullValue;
    return nullValue;
}

inline int64_t asInt64Safe(const boost::json::value &v)
{
    if (v.is_int64())
    {
        return v.as_int64();
    }
    if (v.is_uint64())
    {
        return static_cast<int64_t>(v.as_uint64());
    }
    return 0;
}

inline uint64_t asUInt64Safe(const boost::json::value &v)
{
    if (v.is_uint64())
    {
        return v.as_uint64();
    }
    if (v.is_int64())
    {
        return static_cast<uint64_t>(v.as_int64());
    }
    return 0;
}

inline double asDoubleSafe(const boost::json::value &v)
{
    if (v.is_double())
    {
        return v.as_double();
    }
    if (v.is_int64())
    {
        return static_cast<double>(v.as_int64());
    }
    if (v.is_uint64())
    {
        return static_cast<double>(v.as_uint64());
    }
    return 0.0;
}

} // namespace detail

inline static const char *ASCSTRING(const boost::json::value &j, const std::string &x, const char *def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_string())
    {
        return v.as_string().c_str();
    }
    return def;
}

inline static std::string ASSTRING(const boost::json::value &j, const std::string &x, const std::string &def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_string())
    {
        return std::string(v.as_string());
    }
    return def;
}

inline static bool ASBOOL(const boost::json::value &j, const std::string &x, bool def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_bool())
    {
        return v.as_bool();
    }
    return def;
}

inline static double ASDOUBLE(const boost::json::value &j, const std::string &x, double def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return detail::asDoubleSafe(v);
    }
    return def;
}

inline static float ASFLOAT(const boost::json::value &j, const std::string &x, float def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return static_cast<float>(detail::asDoubleSafe(v));
    }
    return def;
}

inline static int ASINT(const boost::json::value &j, const std::string &x, int def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return static_cast<int>(detail::asInt64Safe(v));
    }
    return def;
}

inline static int64_t ASINT64(const boost::json::value &j, const std::string &x, int64_t def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return detail::asInt64Safe(v);
    }
    return def;
}

inline static unsigned int ASUINT(const boost::json::value &j, const std::string &x, unsigned int def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return static_cast<unsigned int>(detail::asUInt64Safe(v));
    }
    return def;
}

inline static uint64_t ASUINT64(const boost::json::value &j, const std::string &x, uint64_t def)
{
    const boost::json::value &v = detail::safeGet(j, x);
    if (v.is_number())
    {
        return detail::asUInt64Safe(v);
    }
    return def;
}

inline static bool ISARRAY(const boost::json::value &j, const std::string &key)
{
    return j.is_object() && j.as_object().contains(key) && detail::safeGet(j, key).is_array();
}

// Returns true if the object contains the given key.
inline static bool hasMember(const boost::json::value &j, const std::string &key)
{
    return j.is_object() && j.as_object().contains(key);
}

// Sets a member on the object `j`, promoting it from null (or any non-object)
// to an empty object first if needed (mirrors jsoncpp's auto-promotion of
// operator[] on a null value).
inline static void setMember(boost::json::value &j, const std::string &key, const boost::json::value &val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    j.as_object()[key] = val;
}

// Overload for string values (std::string and const char*).
inline static void setMember(boost::json::value &j, const std::string &key, const std::string &val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    j.as_object()[key] = boost::json::value(boost::json::string_view(val));
}

// Overload for any other type convertible to boost::json::value (int, bool,
// double, boost::json::value, ...). Not used for std::string.
template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::string>>>
inline static void setMember(boost::json::value &j, const std::string &key, T &&val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    boost::json::value v = std::forward<T>(val);
    j.as_object()[key] = std::move(v);
}

// Non-const accessor that returns a reference to the object view of `j`.
// The caller is responsible for ensuring `j` is an object.
inline static boost::json::object &asObjectRef(boost::json::value &j)
{
    return j.as_object();
}

// --- Funciones para extraer valores de un valor JSON directo (sin clave) ---
// Los macros _D parecen asumir que 'j' ya es el valor específico, no el objeto padre.

inline static const char *ASCSTRING_D(const boost::json::value &j, const char *def)
{
    return j.is_string() ? j.as_string().c_str() : def;
}

inline static std::string ASSTRING_D(const boost::json::value &j, const std::string &def)
{
    return j.is_string() ? std::string(j.as_string().c_str()) : def;
}

inline static bool ASBOOL_D(const boost::json::value &j, bool def)
{
    return j.is_bool() ? j.as_bool() : def;
}

inline static double ASDOUBLE_D(const boost::json::value &j, double def)
{
    return j.is_number() ? detail::asDoubleSafe(j) : def;
}

inline static float ASFLOAT_D(const boost::json::value &j, float def)
{
    return j.is_number() ? static_cast<float>(detail::asDoubleSafe(j)) : def;
}

inline static int ASINT_D(const boost::json::value &j, int def)
{
    return j.is_number() ? static_cast<int>(detail::asInt64Safe(j)) : def;
}

inline static int64_t ASINT64_D(const boost::json::value &j, int64_t def)
{
    return j.is_number() ? detail::asInt64Safe(j) : def;
}

inline static unsigned int ASUINT_D(const boost::json::value &j, unsigned int def)
{
    return j.is_number() ? static_cast<unsigned int>(detail::asUInt64Safe(j)) : def;
}

inline static uint64_t ASUINT64_D(const boost::json::value &j, uint64_t def)
{
    return j.is_number() ? detail::asUInt64Safe(j) : def;
}

inline static bool ISARRAY_D(const boost::json::value &j)
{
    return j.is_array();
}

// --- Funciones para extraer valores de un array JSON por índice ---

inline static std::string ARRAY_ASSTRING(const boost::json::value &j, size_t i, const std::string &def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_string())
        {
            return std::string(el.as_string());
        }
    }
    return def;
}

inline static bool ARRAY_ASBOOL(const boost::json::value &j, size_t i, bool def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_bool())
        {
            return el.as_bool();
        }
    }
    return def;
}

inline static double ARRAY_ASDOUBLE(const boost::json::value &j, size_t i, double def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return detail::asDoubleSafe(el);
        }
    }
    return def;
}

inline static float ARRAY_ASFLOAT(const boost::json::value &j, size_t i, float def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return static_cast<float>(detail::asDoubleSafe(el));
        }
    }
    return def;
}

inline static int ARRAY_ASINT(const boost::json::value &j, size_t i, int def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return static_cast<int>(detail::asInt64Safe(el));
        }
    }
    return def;
}

inline static int64_t ARRAY_ASINT64(const boost::json::value &j, size_t i, int64_t def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return detail::asInt64Safe(el);
        }
    }
    return def;
}

inline static unsigned int ARRAY_ASUINT(const boost::json::value &j, size_t i, unsigned int def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return static_cast<unsigned int>(detail::asUInt64Safe(el));
        }
    }
    return def;
}

inline static uint64_t ARRAY_ASUINT64(const boost::json::value &j, size_t i, uint64_t def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_number())
        {
            return detail::asUInt64Safe(el);
        }
    }
    return def;
}

/**
 * Resolves a jsoncpp-style JSON path expression against a JSON value.
 *
 * Supported syntax (subset of Json::Path, matching the features actually used
 * by the library):
 *   - Root token: "." or "$" (both are ignored).
 *   - Member access by name: `.name` or `["name"]` (also `['name']`).
 *   - Array index access: `[0]`.
 *
 * Resolution is performed left-to-right. If a step fails (wrong type or
 * out-of-range), the result is null, mirroring Json::Path::resolve().
 */

class Path
{
public:
    explicit Path(std::string expr)
        : m_expr(std::move(expr))
    {}

    [[nodiscard]] const boost::json::value *resolve(const boost::json::value &root) const noexcept
    {
        std::string_view path = m_expr;
        const auto *cur = &root;

        if (path.empty() || path == "." || path == "$")
        {
            return cur;
        }

        if (path.front() == '$')
        {
            path.remove_prefix(1);
        }

        auto skipSpaces = [](std::string_view &s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
            {
                s.remove_prefix(1);
            }
        };

        auto member = [&](std::string_view key) -> bool
        {
            const auto *obj = cur->if_object();
            cur = obj ? obj->if_contains({key.data(), key.size()}) : nullptr;
            return cur != nullptr;
        };

        bool first = true;

        while (true)
        {
            skipSpaces(path);
            if (path.empty())
            {
                return cur;
            }

            if (path.front() == '.')
            {
                path.remove_prefix(1);
                if (path.empty() || path.front() == '[')
                {
                    return nullptr;
                }
            }
            else if (path.front() != '[' && !first)
            {
                return nullptr;
            }

            if (path.front() != '[')
            {
                const auto len = path.find_first_of(".[ \t]");
                const auto key = path.substr(0, len);

                if (key.empty() || !member(key))
                {
                    return nullptr;
                }

                path.remove_prefix(key.size());
            }
            else
            {
                path.remove_prefix(1);
                skipSpaces(path);

                if (path.empty())
                {
                    return nullptr;
                }

                if (path.front() == '"' || path.front() == '\'')
                {
                    const char quote = path.front();
                    path.remove_prefix(1);

                    const auto end = path.find(quote);
                    if (end == std::string_view::npos)
                    {
                        return nullptr;
                    }

                    const auto key = path.substr(0, end);
                    path.remove_prefix(end + 1);

                    skipSpaces(path);
                    if (path.empty() || path.front() != ']')
                    {
                        return nullptr;
                    }

                    path.remove_prefix(1);

                    if (!member(key))
                    {
                        return nullptr;
                    }
                }
                else
                {
                    const auto end = path.find(']');
                    if (end == std::string_view::npos)
                    {
                        return nullptr;
                    }

                    auto token = path.substr(0, end);
                    while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
                    {
                        token.remove_suffix(1);
                    }

                    std::size_t index = 0;
                    const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), index);

                    if (token.empty() || ec != std::errc{} || ptr != token.data() + token.size())
                    {
                        return nullptr;
                    }

                    path.remove_prefix(end + 1);

                    const auto *arr = cur->if_array();
                    if (!arr || index >= arr->size())
                    {
                        return nullptr;
                    }

                    cur = &(*arr)[index];
                }
            }

            first = false;
        }
    }

private:
    std::string m_expr;
};

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
     * @return A list of strings containing the elements of the JSON array.
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
 * This function takes a set of strings and converts it into a JSON array, where each
 * string in the set becomes an element in the resulting JSON array. The order of elements
 * in the JSON array is determined by the iteration order of the set.
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
 *
 * @return True if the document was parsed successfully, false otherwise.
 */
bool parse(const std::string &document, boost::json::value &root);

/**
 * Returns the error message from the last failed parse() call.
 */
std::string lastParseError();

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
