#include "hdr_cachecontrol.h"

#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantids::Protocols::HTTP::Headers;
using namespace std;
using namespace boost;
using namespace boost::algorithm;

CacheControl::CacheControl()
{
    setDefaults();
}

void CacheControl::setDefaults()
{
    optionNoStore=true;
    optionNoCache=false;
    optionMustRevalidate=false;
    optionPrivate=false;
    optionPublic=false;
    optionImmutable=false;
    optionNoTransform=false;
    optionProxyRevalidate=false;
    maxAge=0;
    sMaxAge=0;
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

    setDefaults();

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

bool CacheControl::getOptionNoStore() const
{
    return optionNoStore;
}

void CacheControl::setOptionNoStore(bool newOptionNoStore)
{
    optionNoStore = newOptionNoStore;
}

bool CacheControl::getOptionNoCache() const
{
    return optionNoCache;
}

void CacheControl::setOptionNoCache(bool newOptionNoCache)
{
    optionNoCache = newOptionNoCache;
}

bool CacheControl::getOptionMustRevalidate() const
{
    return optionMustRevalidate;
}

void CacheControl::setOptionMustRevalidate(bool newOptionMustRevalidate)
{
    optionMustRevalidate = newOptionMustRevalidate;
}

bool CacheControl::getOptionPrivate() const
{
    return optionPrivate;
}

void CacheControl::setOptionPrivate(bool newOptionPrivate)
{
    optionPrivate = newOptionPrivate;
}

bool CacheControl::getOptionPublic() const
{
    return optionPublic;
}

void CacheControl::setOptionPublic(bool newOptionPublic)
{
    optionPublic = newOptionPublic;
}

uint32_t CacheControl::getMaxAge() const
{
    return maxAge;
}

void CacheControl::setMaxAge(uint32_t newMaxAge)
{
    maxAge = newMaxAge;
}

bool CacheControl::getOptionImmutable() const
{
    return optionImmutable;
}

void CacheControl::setOptionImmutable(bool newOptionImmutable)
{
    optionImmutable = newOptionImmutable;
}

bool CacheControl::getOptionNoTransform() const
{
    return optionNoTransform;
}

void CacheControl::setOptionNoTransform(bool newOptionNoTransform)
{
    optionNoTransform = newOptionNoTransform;
}

uint32_t CacheControl::getSMaxAge() const
{
    return sMaxAge;
}

void CacheControl::setSMaxAge(uint32_t newSMaxAge)
{
    sMaxAge = newSMaxAge;
}
