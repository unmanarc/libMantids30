#pragma once

/**
 * Internal detail helpers for JSON value manipulation.
 */

#include <boost/json.hpp>
#include <cstdint>
#include <string_view>



namespace Mantids30::Helpers::JSON {

// Returns the member value of an object, or a null value if the object does
// not contain the key (mirrors jsoncpp's non-throwing operator[] behavior).
inline const boost::json::value &MEMBER_OR_NULL(const boost::json::value &obj, std::string_view key){
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

inline int64_t ASINT64_OR_ZERO(const boost::json::value &v)
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

inline uint64_t ASUINT64_OR_ZERO(const boost::json::value &v)
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

} // namespace Mantids30::Helpers::JSON::detail


