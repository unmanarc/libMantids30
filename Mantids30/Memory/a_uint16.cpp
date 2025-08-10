#include "a_uint16.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT16::UINT16()
{
    setVarType(TYPE_UINT16);
}

UINT16::UINT16(const uint16_t &value)
{
    setVarType(TYPE_UINT16);
    this->m_value = value;
}

uint16_t UINT16::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool UINT16::setValue(const uint16_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT16::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT16::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }

    this->m_value = static_cast<uint16_t>(strtoul(value.c_str(), nullptr, 10));

    if (value != "0" && this->m_value == 0)
        return false;

    return true;
}

std::shared_ptr<Var> UINT16::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<UINT16>();
    if (var)
        *var = this->m_value;
    return var;
}

json UINT16::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (getIsNull())
        return Json::nullValue;

    return m_value;
}

bool UINT16::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = JSON_ASUINT_D(value, 0);
    return true;
}
