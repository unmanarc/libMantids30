#include "a_uint16.h"
#include <stdexcept>      // std::invalid_argument
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

UINT16::UINT16()
{
    value = 0;
    setVarType(TYPE_UINT16);
}

UINT16::UINT16(const uint16_t &value)
{
    setVarType(TYPE_UINT16);
    this->value = value;
}

uint16_t UINT16::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool UINT16::setValue(const uint16_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string UINT16::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool UINT16::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = static_cast<uint16_t>(strtoul( value.c_str(), nullptr, 10 ));

    if (value!="0" && this->value==0) return false;

    return true;
}

Var *UINT16::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    UINT16 * var = new UINT16;
    if (var) *var = this->value;
    return var;
}
