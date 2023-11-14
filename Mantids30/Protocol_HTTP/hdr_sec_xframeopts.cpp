#include "hdr_sec_xframeopts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids30::Network::Protocols::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XFrameOpts::XFrameOpts()
{
    setDefaults();
}

XFrameOpts::XFrameOpts(const eOptsValues &value, const string &allowFromURL)
{
    this->m_value = value;
    this->m_allowFromURL = allowFromURL;

    if (!m_allowFromURL.empty() && value!=ALLOWFROM)
    {
        throw runtime_error("Using allowFromURL without ALLOWFROM");
    }
}

void XFrameOpts::setDefaults()
{
    m_allowFromURL = "";
    m_value = DENY;
}

bool XFrameOpts::isNotActivated() const
{
    return m_value == NONE;
}

string Mantids30::Network::Protocols::HTTP::Headers::Security::XFrameOpts::toString()
{
    switch (m_value)
    {
    case DENY:
        return "DENY";
        break;
    case ALLOWFROM:
        return "ALLOW-FROM " + m_allowFromURL;
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

    setDefaults();

    if (parts.empty())
    {
        m_value = DENY;
        return false;
    }
    else if ( iequals(parts[0],"DENY") )
    {
        m_value = DENY;
    }
    else if ( iequals(parts[0],"SAMEORIGIN") )
    {
        m_value = DENY;
    }
    else if ( iequals(parts[0],"ALLOW-FROM") && parts.size()>=2 )
    {
        m_value = DENY;
        m_allowFromURL = parts[1];
    }
    else
    {
        m_value = DENY;
        return false;
    }

    return true;
}

XFrameOpts::eOptsValues XFrameOpts::getValue() const
{
    return m_value;
}
