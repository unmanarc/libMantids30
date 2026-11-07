#include "a_datetime.h"

#include <Mantids30/Helpers/mem.h>
#include <mutex>

#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Mantids30::Memory::Abstract;
using namespace std;

DATETIME::DATETIME()
{
    setVarType(Type::DATETIME);
}

DATETIME::DATETIME(const time_t &value)
{
    this->m_value = value;

    setVarType(Type::DATETIME);
}

bool DATETIME::isInFuture()
{
    time_t now;
    time(&now);
    return getValue() > now;
}

time_t DATETIME::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

bool DATETIME::setValue(const time_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

string DATETIME::toStringLcl()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return getPlainLclTimeStr(m_value);
}

std::string DATETIME::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return getISOTimeStr(m_value);
}

bool DATETIME::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = fromISOTimeStr(value);

    return true;
}

boost::json::value DATETIME::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return static_cast<uint64_t>(m_value);
}

bool DATETIME::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASINT64_D(value, 0);
    return true;
}

std::shared_ptr<Var> DATETIME::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<DATETIME> var = std::make_shared<DATETIME>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

string DATETIME::getPlainLclTimeStr(time_t v)
{
    char sTime[64];
    sTime[64 - 1] = 0;

    v -= timezone;

    tm tmTime{};
#ifdef _WIN32
    gmtime_s(&tmTime, &v);
#else
    gmtime_r(&v, &tmTime);
#endif
    strftime(sTime, 63, "%F %T", &tmTime);

    return sTime;
}

string DATETIME::getISOTimeStr(const time_t &v)
{
    char sTime[64];
    sTime[64 - 1] = 0;

    tm tmTime{};
#ifdef _WIN32
    gmtime_s(&tmTime, &v);
#else
    gmtime_r(&v, &tmTime);
#endif
    strftime(sTime, 63, "%FT%TZ", &tmTime);

    //    std::strftime(sTime, 63, "%FT%TZ", std::gmtime(&v));
    return sTime;
}

time_t DATETIME::fromISOTimeStr(const std::string_view v)
{
    const std::string s(v);
    tm t{};
    int year, month, day, hour, minute;
    float seconds;
    int consumed = 0;

    if (sscanf(s.c_str(), "%d-%d-%d%*1[T ]%d:%d:%f%n", &year, &month, &day, &hour, &minute, &seconds, &consumed) != 6)
    {
        return 0;
    }

    if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || !std::isfinite(seconds) || seconds < 0 || seconds >= 60)
    {
        return 0;
    }

    int offset = 0;

    if (consumed < s.size())
    {
        if (s[consumed] == 'Z')
        {
            if (consumed + 1 != s.size())
            {
                return 0;
            }
        }
        else
        {
            char sign;
            int h, m, n = 0;

            if (sscanf(s.c_str() + consumed, "%c%2d:%2d%n", &sign, &h, &m, &n) != 3 || (sign != '+' && sign != '-') || h > 23 || m > 59 || consumed + n != s.size())
            {
                return 0;
            }

            offset = (sign == '+' ? 1 : -1) * (h * 3600 + m * 60);
        }
    }

    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = static_cast<int>(seconds);

    const time_t result = timegm(&t);

    // Reject invalid dates normalized by timegm(), e.g. February 31.
    if (t.tm_year != year - 1900 || t.tm_mon != month - 1 || t.tm_mday != day)
    {
        return 0;
    }

    return result - offset;
}