#include "hdr_sec_xframeopts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace CX2::Network::HTTP::Headers::Security;
using namespace CX2::Network::HTTP;
using namespace CX2;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XFrameOpts::XFrameOpts()
{
    setDefaults();
}

XFrameOpts::XFrameOpts(const eXFrameOptsValues &value, const string &fromURL)
{
    this->value = value;
    this->fromURL = fromURL;

    if (!fromURL.empty() && value!=HTTP_XFRAME_ALLOWFROM)
    {
        throw runtime_error("Using fromURL without HTTP_XFRAME_ALLOWFROM");
    }
}

void XFrameOpts::setDefaults()
{
    fromURL = "";
    value = HTTP_XFRAME_DENY;
}

bool XFrameOpts::isNotActivated() const
{
    return value == HTTP_XFRAME_NONE;
}

string CX2::Network::HTTP::Headers::Security::XFrameOpts::toValue()
{
    switch (value)
    {
    case HTTP_XFRAME_DENY:
        return "DENY";
        break;
    case HTTP_XFRAME_ALLOWFROM:
        return "ALLOW-FROM " + fromURL;
        break;
    case HTTP_XFRAME_SAMEORIGIN:
        return "SAMEORIGIN";
        break;
    case HTTP_XFRAME_NONE:
    default:
        return "";
        break;
    }
}

bool XFrameOpts::fromValue(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of(" "),token_compress_on);

    setDefaults();

    if (parts.empty())
    {
        value = HTTP_XFRAME_DENY;
        return false;
    }
    else if ( iequals(parts[0],"DENY") )
    {
        value = HTTP_XFRAME_DENY;
    }
    else if ( iequals(parts[0],"SAMEORIGIN") )
    {
        value = HTTP_XFRAME_DENY;
    }
    else if ( iequals(parts[0],"ALLOW-FROM") && parts.size()>=2 )
    {
        value = HTTP_XFRAME_DENY;
        fromURL = parts[1];
    }
    else
    {
        value = HTTP_XFRAME_DENY;
        return false;
    }

    return true;
}

string XFrameOpts::getFromURL() const
{
    return fromURL;
}

void XFrameOpts::setFromURL(const string &value)
{
    fromURL = value;
}

eXFrameOptsValues XFrameOpts::getValue() const
{
    return value;
}
