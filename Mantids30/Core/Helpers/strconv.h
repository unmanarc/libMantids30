#pragma once

#include <string>

namespace Mantids30::Core::Helpers {

/**
 * Provides utility functions for working with string conversions.
 */
class StringConversions
{
public:
    static size_t getSizeFromString(const std::string &szString);
};

} // namespace Mantids30::Core::Helpers
