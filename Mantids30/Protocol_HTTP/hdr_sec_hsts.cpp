#include "hdr_sec_hsts.h"
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>


using namespace Mantids30::Network::Protocols::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

HSTS::HSTS()
{
    setDefaults();
}

HSTS::HSTS(uint32_t maxAge, bool includeSubDomains, bool preload)
{
    this->isActivated=true;
    this->isPreloadEnabled=preload;
    this->isSubdomainIncluded=includeSubDomains;
    this->m_maxAge=maxAge;
}

void HSTS::setDefaults()
{
    isActivated=false;
    isPreloadEnabled=false;
    isSubdomainIncluded=false;
    m_maxAge=0;
}

std::string HSTS::toString()
{
    if (isActivated)
    {
        string r = "";

        if (m_maxAge)
            r+= "; max-age=" + std::to_string(m_maxAge);
        if (isSubdomainIncluded)
            r+= "; includeSubDomains";
        if (isPreloadEnabled)
            r+= "; preload";

        if (r.size()>2)
            return r.substr(2);
    }
    return "";
}

bool HSTS::fromString(const std::string &sValue)
{
    vector<string> parts;
    split(parts,sValue,is_any_of("; "),token_compress_on);

    setDefaults();

    if (sValue.empty())
        isActivated = false;
    else
    {
        isActivated = true;
        for ( size_t i=0; i<parts.size();i++ )
        {
            if (iequals(parts[i],"preload"))
                isPreloadEnabled = true;
            else if (iequals(parts[i],"includeSubDomains"))
                isSubdomainIncluded = true;
            else if (istarts_with(parts[i],"max-age="))
                m_maxAge = strtoul(parts[i].substr(0).c_str(),nullptr,10);
        }
    }
    return true;
}
