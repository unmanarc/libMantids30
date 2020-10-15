#include "http_date.h"

#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

using namespace CX2::Network::HTTP;
using namespace CX2;
using namespace boost::posix_time;

HTTP_Date::HTTP_Date()
{
    setCurrentTime();
}

time_t HTTP_Date::getRawTime() const
{
    return rawTime;
}

void HTTP_Date::setRawTime(const time_t &value)
{
    rawTime = value;
}

std::string HTTP_Date::toString()
{
    char buffer[64];
    struct tm timeinfo;

    // Win32/Linux Compatible (not using localtime_r):
    ptime t = from_time_t(rawTime);
    timeinfo = to_tm(t);

    strftime (buffer,80,"%a, %d %b %Y %T %Z",&timeinfo);
    return std::string(buffer);
}

bool HTTP_Date::fromString(const std::string &fTime)
{
    //    if (strptime(fTime.c_str(), "%a, %d %b %Y %T %Z", &tm) == nullptr) return false;
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
    // TODO: check this function (specially with different locales).
    // TODO: check hour zones changes.
}

void HTTP_Date::setCurrentTime()
{
    rawTime = time(nullptr);
}

void HTTP_Date::incTime(const uint32_t &seconds)
{
    rawTime += seconds;
}
