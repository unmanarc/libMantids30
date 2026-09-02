#include "hdr_etag.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantids30::Network::Protocol::HTTP::Headers;

std::string ETag::toString() const
{
    if (value.empty())
    {
        return "";
    }
    if (weak)
    {
        return "W/\"" + value + "\"";
    }
    return "\"" + value + "\"";
}

void ETag::fromString(const std::string &str)
{
    value.clear();
    weak = false;

    if (str.empty())
    {
        return;
    }

    std::string s = str;
    // Trim whitespace
    boost::algorithm::trim(s);

    if (s.size() >= 2 && s[0] == '"')
    {
        // Strong ETag: "value"
        value = s.substr(1, s.size() - 2);
    }
    else if (s.size() >= 5 && boost::algorithm::istarts_with(s, "W/\""))
    {
        // Weak ETag: W/"value"
        weak = true;
        value = s.substr(3, s.size() - 4);
    }
    else
    {
        // No quotes, use as-is
        value = s;
    }
}

bool ETag::matches(const ETag &other) const
{
    if (value.empty() || other.value.empty())
    {
        return false;
    }
    // Weak comparison: just compare the tag values (RFC 7232 Section 2.3.2)
    return (value == other.value);
}

std::string ETag::getValue() const
{
    return value;
}

void ETag::setValue(const std::string &newValue)
{
    value = newValue;
}

bool ETag::getWeak() const
{
    return weak;
}

void ETag::setWeak(bool newWeak)
{
    weak = newWeak;
}

void ETag::clear()
{
    *this = ETag();
}