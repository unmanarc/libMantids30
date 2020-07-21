#include "a_string.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Vars;

A_STRING::A_STRING()
{
    setVarType(ABSTRACT_STRING);
}

A_STRING::~A_STRING()
{
}

std::string A_STRING::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool A_STRING::setValue(const std::string &value)
{
    return fromString(value);
}

std::string A_STRING::toString()
{
    return getValue();
}

bool A_STRING::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
    return true;
}

Abstract *A_STRING::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);
    A_STRING * var = new A_STRING;
    if (var) *var = this->value;
    return var;
}
