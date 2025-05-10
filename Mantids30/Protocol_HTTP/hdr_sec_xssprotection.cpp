#include "hdr_sec_xssprotection.h"
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids30::Network::Protocols::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

XSSProtection::XSSProtection()
{
}

string XSSProtection::toString()
{
    if (!isActivated)
    {
        return "0";
    }
    else
    {
        string r = "1";
        if (enableBlockingMode)
            r+= "; mode=block";
        if (!reportURL.empty())
            r+= "; report=" + reportURL;
        return r;
    }
}

bool XSSProtection::fromString(const string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of("; "),token_compress_on);

    *this=XSSProtection();

    if (parts.empty())
    {
        isActivated = false;
    }
    else if (parts.size() == 1)
    {
        isActivated = parts[0]=="1";
    }
    else if (parts.size() >= 2)
    {
        isActivated = parts[0]=="1";
        if (isActivated)
        {
            enableBlockingMode = false;
            for ( size_t i=1; i<parts.size();i++ )
            {
                if (iequals(parts[i],"mode=block"))
                    enableBlockingMode = true;
                else if (istarts_with(parts[i],"report="))
                    reportURL = parts[i].substr(7);
            }
        }
    }
    return true;
}
