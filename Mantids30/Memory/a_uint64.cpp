#include "a_uint64.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT64::UINT64()
{
    setVarType(TYPE_UINT64);
}

UINT64::UINT64(const uint64_t &value)
{
    setVarType(TYPE_UINT64);
    this->m_value = value;
}

uint64_t UINT64::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

int64_t UINT64::getIValueTruncatedOrZero()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (m_value <= 0x7FFFFFFFFFFFFFFF)
        return m_value;
    else
        return 0;
}

bool UINT64::setValue(const uint64_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT64::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT64::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }

    this->m_value = strtoull(value.c_str(), nullptr, 10);
    if (value != "0" && this->m_value == 0)
        return false;

    return true;
}

std::shared_ptr<Var> UINT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<UINT64>();
    if (var)
        *var = this->m_value;
    return var;
}

json UINT64::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (isNull())
        return Json::nullValue;

    return m_value;
}

bool UINT64::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = JSON_ASUINT64_D(value, 0);
    return true;
}
