#pragma once

/**
 * Direct value accessors (no key lookup) for JSON data.
 * These functions operate on a value that is already the specific element,
 * not on a parent object with a key.
 */

#include "json_detail.h"

#include <boost/json.hpp>
#include <cstdint>

namespace Mantids30::Helpers::JSON {

inline static const char *ASCSTRING_D(const boost::json::value &j, const char *def)
{
    return j.is_string() ? j.as_string().c_str() : def;
}

inline static std::string_view ASSTRING_D(const boost::json::value &j, std::string_view def = {})
{
    if (j.is_string())
    {
        const auto &s = j.as_string();
        return {s.data(), s.size()};
    }
    return def;
}

inline static bool ASBOOL_D(const boost::json::value &j, bool def)
{
    return j.is_bool() ? j.as_bool() : def;
}

inline static double ASDOUBLE_D(const boost::json::value &j, double def)
{
    return j.is_number() ? ASDOUBLE_OR_ZERO(j) : def;
}

inline static float ASFLOAT_D(const boost::json::value &j, float def)
{
    return j.is_number() ? static_cast<float>(ASDOUBLE_OR_ZERO(j)) : def;
}

inline static int ASINT_D(const boost::json::value &j, int def)
{
    return j.is_number() ? static_cast<int>(ASINT64_OR_ZERO(j)) : def;
}

inline static int64_t ASINT64_D(const boost::json::value &j, int64_t def)
{
    return j.is_number() ? ASINT64_OR_ZERO(j) : def;
}

inline static unsigned int ASUINT_D(const boost::json::value &j, unsigned int def)
{
    return j.is_number() ? static_cast<unsigned int>(ASUINT64_OR_ZERO(j)) : def;
}

inline static uint64_t ASUINT64_D(const boost::json::value &j, uint64_t def)
{
    return j.is_number() ? ASUINT64_OR_ZERO(j) : def;
}

} // namespace Mantids30::Helpers::JSON
