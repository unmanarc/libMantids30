#pragma once

/**
 * Object member accessors for JSON data.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Mantids30::DataFormat::JSON {

inline static const char *ASCSTRING(const boost::json::value &j, std::string_view x, const char *def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_string())
    {
        return v.as_string().c_str();
    }
    return def;
}

inline static std::string_view ASSTRING(const boost::json::value &j, std::string_view x, std::string_view def = {})
{
    const auto &v = MEMBER_OR_NULL(j, x);

    if (v.is_string())
    {
        const auto &s = v.as_string();
        return {s.data(), s.size()};
    }

    return def;
}

inline static bool ASBOOL(const boost::json::value &j, std::string_view x, bool def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_bool())
    {
        return v.as_bool();
    }
    return def;
}

inline static double ASDOUBLE(const boost::json::value &j, std::string_view x, double def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_number())
    {
        return ASDOUBLE_OR_ZERO(v);
    }
    return def;
}

inline static float ASFLOAT(const boost::json::value &j, std::string_view x, float def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_number())
    {
        return static_cast<float>(ASDOUBLE_OR_ZERO(v));
    }
    return def;
}

inline static int ASINT(const boost::json::value &j, std::string_view x, int def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_number())
    {
        return static_cast<int>(ASINT64_OR_ZERO(v));
    }
    return def;
}

inline static int64_t ASINT64(const boost::json::value &j, std::string_view x, int64_t def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_number())
    {
        return ASINT64_OR_ZERO(v);
    }
    return def;
}

inline static unsigned int ASUINT(const boost::json::value &j, std::string_view x, unsigned int def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_number())
    {
        return static_cast<unsigned int>(ASUINT64_OR_ZERO(v));
    }
    return def;
}

// Returns def unless the member is an integer representable as uint64_t.
inline static uint64_t ASUINT64(const boost::json::value &j, std::string_view x, uint64_t def)
{
    const boost::json::value &v = MEMBER_OR_NULL(j, x);
    if (v.is_uint64())
    {
        return v.as_uint64();
    }
    if (v.is_int64() && v.as_int64() >= 0)
    {
        return static_cast<uint64_t>(v.as_int64());
    }
    return def;
}

// Returns true if the object contains the given key.
inline static bool HAS_MEMBER(const boost::json::value &j, std::string_view key)
{
    return j.is_object() && j.as_object().contains({key.data(), key.size()});
}

// Sets a member on the object `j`, promoting it from null (or any non-object)
// to an empty object first if needed (mirrors jsoncpp's auto-promotion of
// operator[] on a null value).

inline static void setMember(boost::json::value &j, std::string_view key, const boost::json::value &val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    j.as_object()[{key.data(), key.size()}] = val;
}

// Overload for string values (std::string and const char*).
inline static void setMember(boost::json::value &j, std::string_view key, const std::string &val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    j.as_object()[{key.data(), key.size()}] = boost::json::value(boost::json::string_view(val.data(), val.size()));
}

// Overload for any other type convertible to boost::json::value (int, bool,
// double, boost::json::value, ...). Not used for std::string.
template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::string>>>
inline static void setMember(boost::json::value &j, std::string_view key, T &&val)
{
    if (!j.is_object())
    {
        j = boost::json::object{};
    }
    boost::json::value v = std::forward<T>(val);
    j.as_object()[{key.data(), key.size()}] = std::move(v);
}

} // namespace Mantids30::DataFormat::JSON
