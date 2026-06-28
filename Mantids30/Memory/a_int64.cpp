#include "a_int64.h"

#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

INT64::INT64()
{
    setVarType(Type::INT64);
}

INT64::INT64(const int64_t &value)
{
    this->m_value = value;

    setVarType(Type::INT64);
}

int64_t INT64::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool INT64::setValue(const int64_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT64::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);
}

bool INT64::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return false;
    }

    char *end = nullptr;
    long long result = strtoll(value.c_str(), &end, 10);

    if (end == value.c_str() || *end != '\0' || result > INT64_MAX || result < INT64_MIN)
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = static_cast<int64_t>(result);
    return true;
}

std::shared_ptr<Var> INT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    std::shared_ptr<INT64> var = std::make_shared<INT64>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

json INT64::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool INT64::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = Helpers::JSON::ASINT64_D(value, 0);
    return true;
}
