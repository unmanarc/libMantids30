#include "a_uint16.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT16::UINT16()
{
    setVarType(Type::UINT16);
}

UINT16::UINT16(const uint16_t &value)
{
    setVarType(Type::UINT16);
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
        return false; // Empty string is not a valid numeric input
    }

    char *end = nullptr;
    unsigned long result = strtoul(value.c_str(), &end, 10);

    // Check for conversion errors:
    // - end == value.c_str(): no digits were found
    // - *end != '\0': trailing non-numeric characters
    // - result > UINT16_MAX: overflow (though strtoul doesn't clamp)
    if (end == value.c_str() || *end != '\0' || result > UINT16_MAX)
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = static_cast<uint16_t>(result);
    return true;
}

std::shared_ptr<Var> UINT16::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    std::shared_ptr<UINT16> var = std::make_shared<UINT16>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

json UINT16::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool UINT16::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = Helpers::JSON::ASUINT_D(value, 0);
    return true;
}
