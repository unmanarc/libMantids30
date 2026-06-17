#include "hdr_sec_hsts.h"
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstdlib>

using namespace Mantids30::Network::Protocol::HTTP::Headers::Security;
using namespace Mantids30::Network::Protocol;
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
    this->isActivated = true;
    this->isPreloadEnabled = preload;
    this->isSubdomainIncluded = includeSubDomains;
    this->m_maxAge = maxAge;
}

void HSTS::setDefaults()
{
    isActivated = false;
    isPreloadEnabled = false;
    isSubdomainIncluded = false;
    m_maxAge = 0;
}

std::string HSTS::toString() const
{
    if (isActivated)
    {
        string r;

        if (m_maxAge)
        {
            r += "; max-age=" + std::to_string(m_maxAge);
        }
        if (isSubdomainIncluded)
        {
            r += "; includeSubDomains";
        }
        if (isPreloadEnabled)
        {
            r += "; preload";
        }

        if (r.size() > 2)
        {
            return r.substr(2);
        }
    }
    return "";
}

bool HSTS::fromString(const std::string &sValue)
{
    // Reset the current object to a clean state
    // This ensures that if parsing fails, the object doesn't hold stale data.
    *this = HSTS();

    vector<string> parts;
    split(parts, sValue, is_any_of("; "), token_compress_on);

    if (sValue.empty())
    {
        isActivated = false;
    }
    else
    {
        isActivated = true;
        for (const auto & part : parts)
        {
            if (iequals(part, "preload"))
            {
                isPreloadEnabled = true;
            }
            else if (iequals(part, "includeSubDomains"))
            {
                isSubdomainIncluded = true;
            }
            else if (istarts_with(part, "max-age="))
            {
                m_maxAge = strtoul(part.substr(0).c_str(), nullptr, 10);
            }
        }
    }
    return true;
}
