#include "strconv.h"

#include "safeint.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>

size_t Mantids30::Helpers::StringConversions::getSizeFromString(const std::string &szString)
{
    // Convert to lowercase for case-insensitive comparison
    std::string sizeStr = boost::to_lower_copy(szString);

    // Remove whitespace
    boost::trim(sizeStr);

    if (sizeStr.empty())
    {
        throw std::invalid_argument("Invalid object size: empty string");
        return 0;
    }

    // Extract the numeric part
    size_t pos = 0;
    while (pos < sizeStr.length() && (isdigit(sizeStr[pos]) || sizeStr[pos] == '.'))
    {
        pos++;
    }

    std::string numberPart = sizeStr.substr(0, pos);
    std::string unitPart = sizeStr.substr(pos);

    // Remove any whitespace from unit part
    boost::trim(unitPart);

    // Convert to double
    char *endPtr;
    long double fileSize = strtold(numberPart.c_str(), &endPtr);

    if (*endPtr != '\0')
    {
        throw std::invalid_argument("Invalid object size: malformed number");
        return 0;
    }

    // Apply unit multiplier
    long double multiplier = 1;
    if (unitPart == "b" || unitPart == "")
    {
        multiplier = 1;
    }
    else if (unitPart == "kb" || unitPart == "k")
    {
        multiplier = 1024;
    }
    else if (unitPart == "mb" || unitPart == "m")
    {
        multiplier = 1024 * 1024;
    }
    else if (unitPart == "gb" || unitPart == "g")
    {
        multiplier = 1024 * 1024 * 1024;
    }
    else if (unitPart == "tb" || unitPart == "t")
    {
        multiplier = (long double) 1024 * (long double) 1024 * (long double) 1024 * (long double) 1024;
    }
    else
    {
        throw std::invalid_argument("Invalid object size: unknown unit");
        return 0;
    }

    // Check for negative number
    if (fileSize < 0)
    {
        throw std::invalid_argument("Invalid object size: negative value");
        return 0;
    }

    // Check for overflow using safeMultiply
    try
    {
        return safeMultiply<long double>(static_cast<uint64_t>(fileSize), multiplier);
    }
    catch (const std::overflow_error &e)
    {
        throw std::overflow_error("Object size too large");
        return 0;
    }

}
