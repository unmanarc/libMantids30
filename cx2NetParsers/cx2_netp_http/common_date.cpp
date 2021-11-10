#include "common_date.h"

#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

using namespace CX2::Network::HTTP;
using namespace CX2::Network::HTTP::Common;
using namespace CX2;
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
    char buffer[64];
    struct tm timeinfo;

    // Win32/Linux Compatible (not using localtime_r):
    ptime t = from_time_t(rawTime);
    timeinfo = to_tm(t);

    strftime (buffer,80,"%a, %d %b %Y %T %Z",&timeinfo);
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
