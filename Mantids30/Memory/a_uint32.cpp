#include "a_uint32.h"
#include <stdexcept>      // std::invalid_argument
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT32::UINT32()
{
    m_value = 0;
    setVarType(TYPE_UINT32);
}

UINT32::UINT32(const uint32_t &value)
{
    setVarType(TYPE_UINT16);
    this->m_value = value;
}

uint32_t UINT32::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool UINT32::setValue(const uint32_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT32::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT32::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }

    this->m_value = static_cast<uint32_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->m_value==0) return false;

    return true;
}

std::shared_ptr<Var> UINT32::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<UINT32>();
    if (var) *var = this->m_value;
    return var;
}
