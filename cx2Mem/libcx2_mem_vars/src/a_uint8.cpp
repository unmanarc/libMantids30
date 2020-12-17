#include "a_uint8.h"
#include <stdexcept>      // std::invalid_argument
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

UINT8::UINT8()
{
    value = 0;
    setVarType(TYPE_UINT8);
}

uint8_t UINT8::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool UINT8::setValue(uint8_t value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string UINT8::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool UINT8::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = static_cast<uint8_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *UINT8::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    UINT8 * var = new UINT8;
    if (var) *var = this->value;
    return var;
}
