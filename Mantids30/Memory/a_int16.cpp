#include "a_int16.h"
#include <stdlib.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

INT16::INT16()
{
    setVarType(TYPE_INT16);

}

INT16::INT16(const int16_t &value)
{
    this->m_value = value;
    setVarType(TYPE_INT16);
}

int16_t INT16::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return m_value;
}

bool INT16::setValue(const int16_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT16::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return std::to_string(m_value);
}

bool INT16::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }
    this->m_value = (int16_t)strtol(value.c_str(),nullptr,10);
    if (value!="0" && this->m_value==0) 
        return false;

    return true;
}

std::shared_ptr<Var> INT16::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<INT16>();
    if (var) *var = this->m_value;
    return var;
}
