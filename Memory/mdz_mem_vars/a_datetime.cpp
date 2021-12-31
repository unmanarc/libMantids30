#include "a_datetime.h"

#include <stdlib.h>
#include <mdz_thr_mutex/lock_shared.h>
#include <time.h>
#include <string.h>
#include <mdz_hlp_functions/mem.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Mantids::Memory::Abstract;
using namespace std;

DATETIME::DATETIME()
{
    value = 0;
    setVarType(TYPE_DATETIME);
}

DATETIME::DATETIME(const time_t &value)
{
    this->value = value;
    setVarType(TYPE_DATETIME);
}

time_t DATETIME::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool DATETIME::setValue(const time_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

string DATETIME::toStringLcl()
{
    Threads::Sync::Lock_RD lock(mutex);
    return getPlainLclTimeStr(value);
}

std::string DATETIME::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return getISOTimeStr(value);
}

bool DATETIME::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return false;
    }

    this->value = fromISOTimeStr(value);

    return true;
}

Var *DATETIME::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    DATETIME * var = new DATETIME;
    if (var) *var = this->value;
    return var;
}

string DATETIME::getPlainLclTimeStr(time_t v)
{
    char sTime[64];
    sTime[64-1]=0;

    v-=timezone;

    tm tmTime;
#ifdef _WIN32
    gmtime_s(&tmTime,&v);
#else
    gmtime_r(&v, &tmTime);
#endif
    strftime(sTime, 63,  "%F %T", &tmTime);

    return std::string(sTime);
}

string DATETIME::getISOTimeStr(const time_t &v){
    char sTime[64];
    sTime[64-1]=0;

    tm tmTime;
#ifdef _WIN32
    gmtime_s(&tmTime,&v);
#else
    gmtime_r(&v, &tmTime);
#endif
    strftime(sTime, 63,  "%FT%TZ", &tmTime);

    //    std::strftime(sTime, 63, "%FT%TZ", std::gmtime(&v));
    return std::string(sTime);
}

time_t DATETIME::fromISOTimeStr(const string &v)
{
    // Thanks to: https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
    tm tmTime;
    ZeroBStruct(tmTime);

    if ( v.find(" ") != std::string::npos )
    {
        if (sscanf(v.c_str(), "%d-%d-%d %d:%d:%d",
                   &(tmTime.tm_year),
                   &(tmTime.tm_mon),
                   &(tmTime.tm_mday),
                   &(tmTime.tm_hour),
                   &(tmTime.tm_min),
                   &(tmTime.tm_sec))>=6)
        {
        }

        tmTime.tm_year -= 1900; // Year since 1900
        tmTime.tm_mon -= 1;     // 0-11
        return mktime (&tmTime) - timezone;

    }
    else if (v.find("T") != std::string::npos)
    {
        float s;
        int tzh = 0, tzm = 0;
        if (sscanf(v.c_str(), "%d-%d-%dT%d:%d:%f%d:%dZ",
                   &(tmTime.tm_year),
                   &(tmTime.tm_mon),
                   &(tmTime.tm_mday),
                   &(tmTime.tm_hour),
                   &(tmTime.tm_min),
                   &s, &tzh, &tzm)>6)
        {
            // Fix the sign on minutes:
            if (tzh < 0) tzm *= -1;
        }

        tmTime.tm_year -= 1900; // Year since 1900
        tmTime.tm_mon -= 1;     // 0-11
        tmTime.tm_sec = (int)s;    // 0-61 (0-60 in C++11)

        return mktime (&tmTime) - timezone;
    }
    return 0;
}
