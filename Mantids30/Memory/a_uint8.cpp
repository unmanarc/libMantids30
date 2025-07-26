#include "a_uint8.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

UINT8::UINT8()
{
    setVarType(TYPE_UINT8);
}

UINT8::UINT8(const uint8_t &value)
{
    this->m_value = value;
    setVarType(TYPE_UINT8);
}

uint8_t UINT8::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool UINT8::setValue(const uint8_t & value)
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
        return true;
    }

    this->m_value = static_cast<uint8_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->m_value==0) 
        return false;

    return true;
}

std::shared_ptr<Var> UINT8::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<UINT8>();
    if (var) *var = this->m_value;
    return var;
}
