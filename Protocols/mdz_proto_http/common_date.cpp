#include "common_date.h"

#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

using namespace Mantids::Protocols::HTTP;
using namespace Mantids::Protocols::HTTP::Common;
using namespace Mantids;
using namespace boost::posix_time;

Date::Date()
{
    setCurrentTime();
}

time_t Date::getRawTime() const
{
    return rawTime;
}

void Date::setRawTime(const time_t &value)
{
    rawTime = value;
}

std::string Date::toString()
{
    char buffer[128];
    struct tm timeinfo;

    // Win32/Linux Compatible (not using localtime_r):
    ptime t = from_time_t(rawTime);
    timeinfo = to_tm(t);

#ifndef _WIN32
//    if (timeinfo.tm_zone)
//        timeinfo.tm_hour+=timeinfo.tm_zone; // TODO: check when tm_zone is not zero
    strftime (buffer,sizeof(buffer),"%a, %d %b %Y %T GMT",&timeinfo);
#else
    strftime (buffer,sizeof(buffer),"%a, %d %b %Y %H:%M:%S GMT",&timeinfo);
#endif

    return std::string(buffer);
}

bool Date::fromString(const std::string &fTime)
{

#ifndef _WIN32
    struct tm timeinfo;
    memset(&timeinfo,0, sizeof(tm));
    if (strptime(fTime.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &timeinfo) != NULL)
    {
        std::time_t tt = std::mktime(&timeinfo);
        rawTime = tt;
        return true;
    }
    else
    {
        rawTime=0;
        return false;
    }
#else
    // win32 compatible version... (from c++11..)
    std::tm t = {};
    // C++11 Win32/Linux Compatible
    std::istringstream ss(fTime);
    //ss.imbue(std::locale("he_IL.utf8"));
    ss >> std::get_time(&t, "%a, %d %b %Y %T %Z");
    if (ss.fail())
    {
        rawTime = time(nullptr);
        return false;
    }
    else
    {
        rawTime = mktime ( &t );
        return true;
    }
#endif


    // TODO: check this function (specially with different locales).
    // TODO: check hour zones changes.
}

void Date::setCurrentTime()
{
    rawTime = time(nullptr);
}

void Date::incTime(const uint32_t &seconds)
{
    rawTime += seconds;
}
