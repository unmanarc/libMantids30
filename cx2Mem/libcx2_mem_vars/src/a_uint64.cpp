#include "a_uint64.h"
#include <stdexcept>      // std::invalid_argument
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

UINT64::UINT64()
{
    value = 0;
    setVarType(TYPE_UINT64);
}

uint64_t UINT64::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool UINT64::setValue(const uint64_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string UINT64::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool UINT64::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtoull( value.c_str(), nullptr, 10 );
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *UINT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    UINT64 * var = new UINT64;
    if (var) *var = this->value;
    return var;
}
