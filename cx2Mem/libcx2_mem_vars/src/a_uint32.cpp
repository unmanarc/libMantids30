#include "a_uint32.h"
#include <stdexcept>      // std::invalid_argument
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

UINT32::UINT32()
{
    value = 0;
    setVarType(TYPE_UINT32);
}

UINT32::UINT32(const uint32_t &value)
{
    setVarType(TYPE_UINT16);
    this->value = value;
}

uint32_t UINT32::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool UINT32::setValue(const uint32_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string UINT32::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool UINT32::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = static_cast<uint32_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *UINT32::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    UINT32 * var = new UINT32;
    if (var) *var = this->value;
    return var;
}
