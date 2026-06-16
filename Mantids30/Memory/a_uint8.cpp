#include "a_uint8.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT8::UINT8()
{
    setVarType(Type::UINT8);
}

UINT8::UINT8(const uint8_t &value)
{
    this->m_value = value;
    setVarType(Type::UINT8);
}

uint8_t UINT8::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool UINT8::setValue(const uint8_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT8::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT8::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return false;
    }

    char* end = nullptr;
    unsigned long result = strtoul(value.c_str(), &end, 10);

    if (end == value.c_str() || *end != '\0' || result > UINT8_MAX)
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = static_cast<uint8_t>(result);
    return true;
}

std::shared_ptr<Var> UINT8::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    std::shared_ptr<UINT8> var = std::make_shared<UINT8>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

json UINT8::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool UINT8::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = JSON_ASUINT_D(value, 0);
    return true;
}
