#include "hdr_sec_hsts.h"
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>


using namespace Mantids29::Network::Protocols::HTTP::Headers::Security;
using namespace Mantids29::Network::Protocols::HTTP;
using namespace Mantids29;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

HSTS::HSTS()
{
    setDefaults();
}

HSTS::HSTS(uint32_t maxAge, bool includeSubDomains, bool preload)
{
    this->m_activated=true;
    this->m_preloadEnabled=preload;
    this->m_subdomainIncluded=includeSubDomains;
    this->m_maxAge=maxAge;
}

void HSTS::setDefaults()
{
    m_activated=false;
    m_preloadEnabled=false;
    m_subdomainIncluded=false;
    m_maxAge=0;
}

std::string HSTS::toString()
{
    if (m_activated)
    {
        string r = "";

        if (m_maxAge)
            r+= "; max-age=" + std::to_string(m_maxAge);
        if (m_subdomainIncluded)
            r+= "; includeSubDomains";
        if (m_preloadEnabled)
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
        m_activated = false;
    else
    {
        m_activated = true;
        for ( size_t i=0; i<parts.size();i++ )
        {
            if (iequals(parts[i],"preload"))
                m_preloadEnabled = true;
            else if (iequals(parts[i],"includeSubDomains"))
                m_subdomainIncluded = true;
            else if (istarts_with(parts[i],"max-age="))
                m_maxAge = strtoul(parts[i].substr(0).c_str(),nullptr,10);
        }
    }
    return true;
}
