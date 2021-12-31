#include "common_version.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids::Network::HTTP;
using namespace Mantids::Network::HTTP::Common;
using namespace Mantids;

Version::Version()
{
    versionMajor = 1;
    versionMinor = 1;
}

void Version::parseVersion(const std::string &version)
{
    vector<string> versionParts;
    split(versionParts,version,is_any_of("/"),token_compress_on);

    if (versionParts.size()!=2)
    {
        versionMinor = 0;
        versionMajor = 1;
    }
    else
    {
        vector<string> versionMinorMajor;
        split(versionMinorMajor,versionParts[1],is_any_of("."),token_compress_on);

        if (versionMinorMajor.size()!=2)
        {
            versionMajor = 1;
            versionMinor = 0;
        }
        else
        {
            versionMajor = strtoul(versionMinorMajor[0].c_str(),nullptr,10);
            versionMinor = strtoul(versionMinorMajor[1].c_str(),nullptr,10);
        }
    }
}

string Version::getHTTPVersionString()
{
    char cHTTPVersion[128];
    snprintf(cHTTPVersion,sizeof(cHTTPVersion),"HTTP/%u.%u", versionMajor, versionMinor);
    std::string sHTTPVersion = cHTTPVersion;
    return sHTTPVersion;
}

uint16_t Version::getVersionMinor() const
{
    return versionMinor;
}

void Version::setVersionMinor(const uint16_t &value)
{
    versionMinor = value;
}

void Version::upgradeMinorVersion(const uint16_t &value)
{
    if (value > versionMinor) versionMinor = value;
}

uint16_t Version::getVersionMajor() const
{
    return versionMajor;
}

void Version::setVersionMajor(const uint16_t &value)
{
    versionMajor = value;
}

