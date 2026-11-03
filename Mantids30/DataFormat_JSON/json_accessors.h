#pragma once

/**
 * Object member accessors for JSON data.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace Mantids30::Helpers::JSON {

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

} // namespace Mantids30::Helpers::JSON
