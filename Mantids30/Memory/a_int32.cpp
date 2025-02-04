#include "a_int32.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;


INT32::INT32()
{
    setVarType(TYPE_INT32);
}

INT32::INT32(const int32_t &value)
{
    this->m_value = value;
    setVarType(TYPE_INT32);
}

int32_t INT32::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool INT32::setValue(const int32_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT32::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    return std::to_string(m_value);

}

bool INT32::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }

    this->m_value = strtol(value.c_str(),nullptr,10);
    if (value!="0" && this->m_value==0) return false;

    return true;
}

std::shared_ptr<Var> INT32::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<INT32>();
    if (var) *var = this->m_value;
    return var;
}
