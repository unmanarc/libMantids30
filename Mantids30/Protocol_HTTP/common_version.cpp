#include "common_version.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <inttypes.h>
#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Network::Protocols::HTTP::Common;
using namespace Mantids30;



void Version::parse(const std::string &version)
{
    vector<string> versionParts;
    split(versionParts,version,is_any_of("/"),token_compress_on);

    if (versionParts.size()!=2)
    {
        m_minorVersion = 0;
        m_majorVersion = 1;
    }
    else
    {
        vector<string> versionMinorMajor;
        split(versionMinorMajor,versionParts[1],is_any_of("."),token_compress_on);

        if (versionMinorMajor.size()!=2)
        {
            m_majorVersion = 1;
            m_minorVersion = 0;
        }
        else
        {
            m_majorVersion = strtoul(versionMinorMajor[0].c_str(),nullptr,10);
            m_minorVersion = strtoul(versionMinorMajor[1].c_str(),nullptr,10);
        }
    }
}

string Version::toString()
{
    char cHTTPVersion[128];
    snprintf(cHTTPVersion,sizeof(cHTTPVersion),"HTTP/%" PRIu16 ".%" PRIu16, m_majorVersion, m_minorVersion);
    std::string sHTTPVersion = cHTTPVersion;
    return sHTTPVersion;
}

uint16_t Version::getMinor() const
{
    return m_minorVersion;
}

void Version::setMinor(const uint16_t &value)
{
    m_minorVersion = value;
}

void Version::upgradeMinor(const uint16_t &value)
{
    if (value > m_minorVersion) m_minorVersion = value;
}

uint16_t Version::getMajor() const
{
    return m_majorVersion;
}

void Version::setMajor(const uint16_t &value)
{
    m_majorVersion = value;
}

