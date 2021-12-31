#include "a_int8.h"
#include <stdexcept>      // std::invalid_argument
using namespace Mantids::Memory::Abstract;
#include <mdz_thr_mutex/lock_shared.h>

INT8::INT8()
{
    value = 0;
    setVarType(TYPE_INT8);
}

INT8::INT8(const int8_t &value)
{
    this->value = value;
    setVarType(TYPE_INT64);
}

int8_t INT8::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool INT8::setValue(const int8_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
    return true;
}

std::string INT8::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return std::to_string(value);
}

bool INT8::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }
    this->value = static_cast<int8_t>(strtol( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *INT8::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    INT8 * var = new INT8;
    if (var) *var = this->value;
    return var;
}
