#include "a_ptr.h"
#include <inttypes.h>
#include <cx2_thr_mutex/lock_shared.h>

#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Abstract;

PTR::PTR()
{
    value = 0;
    setVarType(TYPE_PTR);
}

PTR::PTR(void *value)
{
    setVarType(TYPE_PTR);
    this->value = value;
}

void * PTR::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool PTR::setValue(void * value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string PTR::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    char ovalue[256];
    void * ptr = value;
    snprintf(ovalue,256,"%.8" PRIXPTR, (uintptr_t)ptr);
    return ovalue;
}

bool PTR::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }
    this->value = (void *)(strtol( value.c_str(), nullptr, 16 ));
    return true;
}

Var *PTR::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    PTR * var = new PTR;
    if (var) *var = this->value;
    return var;
}
