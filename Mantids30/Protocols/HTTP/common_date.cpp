#include "common_date.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstring>
#ifdef _WIN32
#include <sstream>
#endif
using namespace Mantids30::Protocols;

using namespace Mantids30;
using namespace boost::posix_time;

Mantids30::Protocols::HTTP::Date::Date()
{
    setCurrentTime();
}

Mantids30::Protocols::HTTP::Date::Date(const time_t &unixTime)
{
    setUnixTime(unixTime);
}

time_t Mantids30::Protocols::HTTP::Date::getUnixTime() const
{
    return m_unixTime;
}

void Mantids30::Protocols::HTTP::Date::setUnixTime(const time_t &value)
{
    m_unixTime = value;
}

std::string Mantids30::Protocols::HTTP::Date::toString() const
{
    char buffer[128];
    struct tm timeinfo;

    // Win32/Linux Compatible (not using localtime_r):
    ptime t = from_time_t(m_unixTime);
    timeinfo = to_tm(t);

#ifndef _WIN32
    //    if (timeinfo.tm_zone)
    //        timeinfo.tm_hour+=timeinfo.tm_zone; // TODO: check when tm_zone is not zero
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %T GMT", &timeinfo);
#else
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &timeinfo);
#endif

    return buffer;
}

bool Mantids30::Protocols::HTTP::Date::fromString(const std::string &fTime)
{
    // Reset the current object to a clean state
    // This ensures that if parsing fails, the object doesn't hold stale data.
    *this = Date();

#ifndef _WIN32
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(tm));
    if (strptime(fTime.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &timeinfo) != nullptr)
    {
        std::time_t tt = std::mktime(&timeinfo);
        m_unixTime = tt;
        return true;
    }
    else
    {
        m_unixTime = 0;
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
        m_unixTime = mktime(&t);
        return true;
    }
#endif

    // TODO: check this function (specially with different locales).
    // TODO: check hour zones changes.
}

void Mantids30::Protocols::HTTP::Date::setCurrentTime()
{
    m_unixTime = time(nullptr);
}

void Mantids30::Protocols::HTTP::Date::incTime(const uint32_t &seconds)
{
    m_unixTime += seconds;
}
