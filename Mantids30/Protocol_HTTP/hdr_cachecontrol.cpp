#include "hdr_cachecontrol.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids30::Network::Protocols::HTTP::Headers;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

CacheControl::CacheControl()
{
}

std::string CacheControl::toString()
{
    std::string r;
    if (optionMustRevalidate)
        r += "must-revalidate";

    if (optionProxyRevalidate)
        r += (r.empty()?"":",") + std::string("proxy-revalidate");

    if (optionNoTransform)
        r += (r.empty()?"":",") + std::string("no-transform");

    if (optionImmutable)
        r += (r.empty()?"":",") + std::string("immutable");

    if (optionNoStore)
        r += (r.empty()?"":",") + std::string("no-store");

    if (optionNoCache)
        r += (r.empty()?"":",") + std::string("no-cache");

    if (optionPrivate)
        r += (r.empty()?"":",") + std::string("private");

    if (optionPublic)
        r += (r.empty()?"":",") + std::string("public");

    if (maxAge)
        r += (r.empty()?"":",") + std::string("max-age=") + std::to_string(maxAge);

    if (sMaxAge)
        r += (r.empty()?"":",") + std::string("s-maxage=") + std::to_string(sMaxAge);


    return r;
}

void CacheControl::fromString(const std::string &str)
{
    vector<string> params;
    split(params,str,is_any_of(", "),token_compress_on);

    // Reset to default values using a temporary instance.
    *this = CacheControl();

    bool firstVal = true;
    for (const string & param : params)
    {
        std::string paramLower = boost::to_lower_copy(param);

        if      (paramLower == "no-store") optionNoStore = true;
        else if (paramLower == "no-cache") optionNoCache = true;
        else if (paramLower == "private") optionPrivate = true;
        else if (paramLower == "public") optionPublic= true;
        else if (paramLower == "immutable") optionImmutable= true;
        else if (paramLower == "proxy-revalidate") optionProxyRevalidate= true;
        else if (paramLower == "no-transform") optionNoTransform= true;
        else if (starts_with(paramLower,"max-age="))
        {
            const char * maxAgeValue = paramLower.c_str()+8;
            maxAge = strtoul(maxAgeValue,nullptr,10);
        }
        else if (starts_with(paramLower,"s-maxage="))
        {
            const char * sMaxAgeValue = paramLower.c_str()+9;
            sMaxAge = strtoul(sMaxAgeValue,nullptr,10);
        }
    }
}
