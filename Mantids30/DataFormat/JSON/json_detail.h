#pragma once

/**
 * Internal detail helpers for JSON value manipulation.
 */

#include <boost/json.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <limits>
#include <type_traits>
#include <cstdint>
#include <string_view>



namespace Mantids30::DataFormat::JSON {

// Returns the member value of an object, or a null value if the object does
// not contain the key (mirrors jsoncpp's non-throwing operator[] behavior).
inline const boost::json::value &MEMBER_OR_NULL(const boost::json::value &obj, std::string_view key){
    if (obj.is_object())
    {
        if (auto it = obj.as_object().find({key.data(), key.size()}); it != obj.as_object().end())
        {
            return it->value();
        }
    }
    static const boost::json::value nullValue;
    return nullValue;
}

// Integer accessors accept only integer JSON values representable in T.
template<typename T>
inline T ASINTEGER_OR_DEFAULT(const boost::json::value &v, T def)
{
    static_assert(std::is_integral_v<T>);
    try
    {
        if (v.is_int64())
        {
            return boost::numeric_cast<T>(v.as_int64());
        }
        if (v.is_uint64())
        {
            return boost::numeric_cast<T>(v.as_uint64());
        }
    }
    catch (const boost::numeric::bad_numeric_cast &)
    {
        return def;
    }
    return def;
}

inline int64_t ASINT64_OR_ZERO(const boost::json::value &v)
{
    return ASINTEGER_OR_DEFAULT<int64_t>(v, 0);
}

inline uint64_t ASUINT64_OR_ZERO(const boost::json::value &v)
{
    return ASINTEGER_OR_DEFAULT<uint64_t>(v, 0);
}

inline double ASDOUBLE_OR_ZERO(const boost::json::value &v)
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

inline float ASFLOAT_OR_DEFAULT(const boost::json::value &v, float def)
{
    if (v.is_number())
    {
        const double value = ASDOUBLE_OR_ZERO(v);
        if (std::isfinite(value) && value >= std::numeric_limits<float>::lowest()
            && value <= std::numeric_limits<float>::max())
        {
            return static_cast<float>(value);
        }
    }
    return def;
}

} // namespace Mantids30::DataFormat::JSON::detail


