#include "hdr_sec_xssprotection.h"
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

XSSProtection::XSSProtection()
{
    activated = true;
    blocking = true;
}

bool XSSProtection::getActivated() const
{
    return activated;
}

void XSSProtection::setActivated(bool value)
{
    activated = value;
}

bool XSSProtection::getBlocking() const
{
    return blocking;
}

void XSSProtection::setBlocking(bool value)
{
    blocking = value;
}

std::string XSSProtection::getReportURL() const
{
    return reportURL;
}

void XSSProtection::setReportURL(const std::string &value)
{
    reportURL = value;
}

string XSSProtection::toValue()
{
    if (!activated)
    {
        return "0";
    }
    else
    {
        string r = "1";
        if (blocking)
            r+= "; mode=block";
        if (!reportURL.empty())
            r+= "; report=" + reportURL;
        return r;
    }
}

bool XSSProtection::fromValue(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of("; "),token_compress_on);

    activated = true;
    blocking = true;
    reportURL = "";

    if (parts.empty())
    {
        activated = false;
    }
    else if (parts.size() == 1)
    {
        activated = parts[0]=="1";
    }
    else if (parts.size() >= 2)
    {
        activated = parts[0]=="1";
        if (activated)
        {
            blocking = false;
            for ( size_t i=1; i<parts.size();i++ )
            {
                if (iequals(parts[i],"mode=block"))
                    blocking = true;
                else if (istarts_with(parts[i],"report="))
                    reportURL = parts[i].substr(7);
            }
        }
    }
    return true;
}
