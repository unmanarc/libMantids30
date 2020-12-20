#include "a_double.h"
#include <stdexcept>      // std::invalid_argument
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

DOUBLE::DOUBLE()
{
    value = 0;
    setVarType(TYPE_DOUBLE);
}

DOUBLE::DOUBLE(const double &value)
{
    setVarType(TYPE_DOUBLE);
    this->value = value;
}

double DOUBLE::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

void DOUBLE::setValue(const double &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
}

std::string DOUBLE::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return std::to_string(value);
}

bool DOUBLE::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    try
    {
        this->value = std::stod( value ) ;
        return true;
    }
    catch( std::invalid_argument * )
    {
        return false;
    }
    catch ( std::out_of_range * )
    {
        return false;
    }
}

Var *DOUBLE::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    DOUBLE * var = new DOUBLE;
    if (var) *var = this->value;
    return var;
}
