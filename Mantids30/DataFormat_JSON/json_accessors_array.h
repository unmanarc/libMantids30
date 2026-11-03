#pragma once

/**
 * Array index accessors for JSON data.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Mantids30::Helpers::JSON {

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

} // namespace Mantids30::Helpers::JSON
