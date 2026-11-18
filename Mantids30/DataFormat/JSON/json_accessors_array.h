#pragma once

/**
 * Array index accessors for JSON data.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Mantids30::DataFormat::JSON {

inline static void ARRAY_APPEND(boost::json::array &v, const boost::json::value &e)
{
    v.emplace_back(e);
}


inline const static boost::json::array &ASARRAY_OR_EMPTY(const boost::json::value &j, std::string_view key)
{
    if (j.is_object() && j.as_object().contains({key.data(), key.size()}))
    {
        const auto &arrayElement = MEMBER_OR_NULL(j, key);
        if (arrayElement.is_array())
        {
            return arrayElement.as_array();
        }
    }

    static const boost::json::array emptyValue;
    return emptyValue;
}

inline const static boost::json::array &ASARRAY_OR_EMPTY(const boost::json::value &j)
{
    if (j.is_array())
    {
        return j.as_array();
    }
    static const boost::json::array emptyValue;
    return emptyValue;
}

inline static std::string_view ARRAY_ASSTRING(const boost::json::value &j, size_t i, std::string_view def)
{
    if (j.is_array() && i < j.as_array().size())
    {
        const boost::json::value &el = j.as_array()[i];
        if (el.is_string())
        {
            const auto &s = el.as_string();
            return {s.data(), s.size()};
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
            return ASDOUBLE_OR_ZERO(el);
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
            return ASFLOAT_OR_DEFAULT(el, def);
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
            return ASINTEGER_OR_DEFAULT<int>(el, def);
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
            return ASINTEGER_OR_DEFAULT<int64_t>(el, def);
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
            return ASINTEGER_OR_DEFAULT<unsigned int>(el, def);
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
            return ASINTEGER_OR_DEFAULT<uint64_t>(el, def);
        }
    }
    return def;
}

} // namespace Mantids30::DataFormat::JSON
