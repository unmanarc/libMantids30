#include "a_int32.h"
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Memory::Abstract;


INT32::INT32()
{
    value = 0;
    setVarType(TYPE_INT32);
}

INT32::INT32(const int32_t &value)
{
    this->value = value;
    setVarType(TYPE_INT32);
}

int32_t INT32::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool INT32::setValue(const int32_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string INT32::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);

}

bool INT32::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtol(value.c_str(),nullptr,10);
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *INT32::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    INT32 * var = new INT32;
    if (var) *var = this->value;
    return var;
}
