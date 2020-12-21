#include "a_datetime.h"

#include <stdlib.h>
#include <cx2_thr_mutex/lock_shared.h>
#include <ctime>


using namespace CX2::Memory::Abstract;
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

string DATETIME::getISOTimeStr(const time_t &v){
    char sTime[64];
    sTime[64-1]=0;
    std::strftime(sTime, 63, "%FT%TZ", std::gmtime(&v));
    return std::string(sTime);
}

time_t DATETIME::fromISOTimeStr(const string &v)
{
    // Thanks to: https://stackoverflow.com/questions/26895428/how-do-i-parse-an-iso-8601-date-with-optional-milliseconds-to-a-struct-tm-in-c
    tm tmTime;

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

    return mktime (&tmTime);
}
