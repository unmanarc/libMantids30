#include "http_security_hsts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>

using namespace CX2::Network::HTTP;
using namespace CX2;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

HTTP_Security_HSTS::HTTP_Security_HSTS()
{
    activated=false;
    preload=false;
    includeSubDomains=false;
    maxAge=0;
}

HTTP_Security_HSTS::HTTP_Security_HSTS(uint32_t maxAge, bool includeSubDomains, bool preload)
{
    this->activated=true;
    this->preload=preload;
    this->includeSubDomains=includeSubDomains;
    this->maxAge=maxAge;
}

bool HTTP_Security_HSTS::getActivated() const
{
    return activated;
}

void HTTP_Security_HSTS::setActivated(bool value)
{
    activated = value;
}

bool HTTP_Security_HSTS::getPreload() const
{
    return preload;
}

void HTTP_Security_HSTS::setPreload(bool value)
{
    preload = value;
}

bool HTTP_Security_HSTS::getIncludeSubDomains() const
{
    return includeSubDomains;
}

void HTTP_Security_HSTS::setIncludeSubDomains(bool value)
{
    includeSubDomains = value;
}

std::string HTTP_Security_HSTS::toValue()
{
    if (activated)
    {
        string r = "";

        if (maxAge)
            r+= "; max-age=" + std::to_string(maxAge);
        if (includeSubDomains)
            r+= "; includeSubDomains";
        if (preload)
            r+= "; preload";

        if (r.size()>2)
            return r.substr(2);
    }
    return "";
}

bool HTTP_Security_HSTS::fromValue(const std::string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of("; "),token_compress_on);

    includeSubDomains = false;
    preload = false;
    maxAge = 0;

    if (sValue.empty())
        activated = false;
    else
    {
        activated = true;
        for ( size_t i=0; i<parts.size();i++ )
        {
            if (iequals(parts[i],"preload"))
                preload = true;
            else if (iequals(parts[i],"includeSubDomains"))
                includeSubDomains = true;
            else if (istarts_with(parts[i],"max-age="))
                maxAge = strtoul(parts[i].substr(0).c_str(),nullptr,10);
        }
    }
    return true;
}
