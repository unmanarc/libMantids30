#include "a_int64.h"
#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Abstract;
#include <cx2_thr_mutex/lock_shared.h>

INT64::INT64()
{
    value = 0;
    setVarType(TYPE_INT64);
}

int64_t INT64::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool INT64::setValue(const int64_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string INT64::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool INT64::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtoll(value.c_str(),nullptr,10);
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *INT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    INT64 * var = new INT64;
    if (var) *var = this->value;
    return var;
}
