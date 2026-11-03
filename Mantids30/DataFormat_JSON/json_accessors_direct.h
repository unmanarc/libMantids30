#pragma once

/**
 * Direct value accessors (no key lookup) for JSON data.
 * These functions operate on a value that is already the specific element,
 * not on a parent object with a key.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <cstdint>
#include <string>

namespace Mantids30::Helpers::JSON {

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

} // namespace Mantids30::Helpers::JSON
