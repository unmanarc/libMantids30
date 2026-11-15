#include "hdr_sec_xframeopts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantids30::Network::Protocol::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XFrameOptions::XFrameOptions(const Option &value, const string &allowFromURL)
{
    this->value = value;
    this->allowFromURL = allowFromURL;

    if (!allowFromURL.empty() && value != Option::ALLOWFROM)
    {
        throw runtime_error("Using allowFromURL without ALLOWFROM");
    }
}

bool XFrameOptions::isNotActivated() const
{
    return value == Option::NONE;
}

string Mantids30::Network::Protocol::HTTP::Headers::Security::XFrameOptions::toString() const
{
    switch (value)
    {
    case Option::DENY:
        return "DENY";
        break;
    case Option::ALLOWFROM:
        return "ALLOW-FROM " + allowFromURL;
        break;
    case Option::SAMEORIGIN:
        return "SAMEORIGIN";
        break;
    case Option::NONE:
    default:
        return "";
        break;
    }
}

bool XFrameOptions::fromString(const string &sValue)
{
    // Reset the current object to a clean state
    // This ensures that if parsing fails, the object doesn't hold stale data.
    *this = XFrameOptions();

    if (sValue.empty())
    {
        return false;
    }

    // Split the header value by space
    // Note: Assuming 'split' and 'is_any_of' are from Boost or a similar utility.
    // If using std::string, you'd need a custom splitter or stringstream.
    vector<string> parts;
    split(parts, sValue, is_any_of(" "), token_compress_on);

    if (parts.empty())
    {
        return false;
    }

    // Normalize the first part to lower case for comparison if iequals doesn't handle it internally
    // Assuming iequals is case-insensitive equality check

    if (iequals(parts[0], "DENY"))
    {
        value = Option::DENY;
        return true;
    }

    if (iequals(parts[0], "SAMEORIGIN"))
    {
        value = Option::SAMEORIGIN;
        return true;
    }

    if (iequals(parts[0], "ALLOW-FROM"))
    {
        if (parts.size() < 2)
        {
            // ALLOW-FROM requires a URL
            return false;
        }

        value = Option::ALLOWFROM;
        allowFromURL = parts[1];
        return true;
    }

    // Unknown or invalid header value
    return false;
}
