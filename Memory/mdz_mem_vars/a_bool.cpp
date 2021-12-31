#include "a_bool.h"
#include <stdexcept>      // std::invalid_argument
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Memory::Abstract;

BOOL::BOOL()
{
    value = false;
    setVarType(TYPE_BOOL);
}

BOOL::BOOL(const bool &value)
{
    setVarType(TYPE_BOOL);
    this->value = value;
}

bool BOOL::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool BOOL::setValue(bool value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
    return true;
}

std::string BOOL::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value?"true":"false";
}

bool BOOL::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (value == "true" || value == "TRUE" || value == "1" || value == "t" || value == "T") this->value = true;
    else this->value = false;
    return true;
}

Var *BOOL::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);
    BOOL * var = new BOOL;
    if (var) *var = this->value;
    return var;
}
