#include "a_int16.h"
#include <stdlib.h>
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Memory::Abstract;

INT16::INT16()
{
    value = 0;
    setVarType(TYPE_INT16);

}

INT16::INT16(const int16_t &value)
{
    this->value = value;
    setVarType(TYPE_INT16);
}

int16_t INT16::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

bool INT16::setValue(const int16_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string INT16::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return std::to_string(value);
}

bool INT16::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }
    this->value = (int16_t)strtol(value.c_str(),nullptr,10);
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *INT16::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    INT16 * var = new INT16;
    if (var) *var = this->value;
    return var;
}
