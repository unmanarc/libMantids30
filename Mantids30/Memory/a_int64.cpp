#include "a_int64.h"
#include <stdexcept>      // std::invalid_argument
using namespace Mantids30::Memory::Abstract;
#include <Mantids30/Threads/lock_shared.h>

INT64::INT64()
{
    m_value = 0;
    setVarType(TYPE_INT64);
}

INT64::INT64(const int64_t &value)
{
    this->m_value = value;
    setVarType(TYPE_INT64);
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
        return true;
    }

    this->m_value = strtoll(value.c_str(),nullptr,10);
    if (value!="0" && this->m_value==0) return false;

    return true;
}

std::shared_ptr<Var> INT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<INT64>();
    if (var) *var = this->m_value;
    return var;
}
