#include "common_date.h"

#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

using namespace Mantids29::Protocols::HTTP;
using namespace Mantids29::Protocols::HTTP::Common;
using namespace Mantids29;
using namespace boost::posix_time;

Date::Date()
{
    setCurrentTime();
}

time_t Date::getUnixTime() const
{
    return m_unixTime;
}

void Date::setUnixTime(const time_t &value)
{
    m_unixTime = value;
}

std::string Date::toString()
{
    char buffer[128];
    struct tm timeinfo;

    // Win32/Linux Compatible (not using localtime_r):
    ptime t = from_time_t(m_unixTime);
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
        m_unixTime = tt;
        return true;
    }
    else
    {
        m_unixTime=0;
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
        m_unixTime = time(nullptr);
        return false;
    }
    else
    {
        m_unixTime = mktime ( &t );
        return true;
    }
#endif


    // TODO: check this function (specially with different locales).
    // TODO: check hour zones changes.
}

void Date::setCurrentTime()
{
    m_unixTime = time(nullptr);
}

void Date::incTime(const uint32_t &seconds)
{
    m_unixTime += seconds;
}
