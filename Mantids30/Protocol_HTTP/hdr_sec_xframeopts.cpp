#include "hdr_sec_xframeopts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids30::Network::Protocols::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XFrameOpts::XFrameOpts()
{
}

XFrameOpts::XFrameOpts(const eOptsValues &value, const string &allowFromURL)
{
    this->value = value;
    this->allowFromURL = allowFromURL;

    if (!allowFromURL.empty() && value!=ALLOWFROM)
    {
        throw runtime_error("Using allowFromURL without ALLOWFROM");
    }
}

bool XFrameOpts::isNotActivated() const
{
    return value == NONE;
}

string Mantids30::Network::Protocols::HTTP::Headers::Security::XFrameOpts::toString()
{
    switch (value)
    {
    case DENY:
        return "DENY";
        break;
    case ALLOWFROM:
        return "ALLOW-FROM " + allowFromURL;
        break;
    case SAMEORIGIN:
        return "SAMEORIGIN";
        break;
    case NONE:
    default:
        return "";
        break;
    }
}

bool XFrameOpts::fromString(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of(" "),token_compress_on);

    *this = XFrameOpts();

    if (parts.empty())
    {
        value = DENY;
        return false;
    }
    else if ( iequals(parts[0],"DENY") )
    {
        value = DENY;
    }
    else if ( iequals(parts[0],"SAMEORIGIN") )
    {
        value = DENY;
    }
    else if ( iequals(parts[0],"ALLOW-FROM") && parts.size()>=2 )
    {
        value = DENY;
        allowFromURL = parts[1];
    }
    else
    {
        value = DENY;
        return false;
    }

    return true;
}
