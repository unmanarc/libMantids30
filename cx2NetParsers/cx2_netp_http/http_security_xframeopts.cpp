#include "http_security_xframeopts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace CX2::Network::HTTP;
using namespace CX2;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

HTTP_Security_XFrameOpts::HTTP_Security_XFrameOpts()
{
    value = HTTP_XFRAME_DENY;
}

HTTP_Security_XFrameOpts::HTTP_Security_XFrameOpts(const eHTTP_Security_XFrameOptsValues &value, const string &fromURL)
{
    this->value = value;
    this->fromURL = fromURL;

    if (!fromURL.empty() && value!=HTTP_XFRAME_ALLOWFROM)
    {
        throw runtime_error("Using fromURL without HTTP_XFRAME_ALLOWFROM");
    }
}

bool HTTP_Security_XFrameOpts::isNotActivated() const
{
    return value == HTTP_XFRAME_NONE;
}

string CX2::Network::HTTP::HTTP_Security_XFrameOpts::toValue()
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

bool HTTP_Security_XFrameOpts::fromValue(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of(" "),token_compress_on);

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

string HTTP_Security_XFrameOpts::getFromURL() const
{
    return fromURL;
}

void HTTP_Security_XFrameOpts::setFromURL(const string &value)
{
    fromURL = value;
}

eHTTP_Security_XFrameOptsValues HTTP_Security_XFrameOpts::getValue() const
{
    return value;
}
