#include "a_double.h"
#include <stdexcept>      // std::invalid_argument

using namespace CX2::Memory::Vars;

A_DOUBLE::A_DOUBLE()
{
    value = 0;
    setVarType(ABSTRACT_DOUBLE);

}

double A_DOUBLE::getValue()
{
    return value;
}

void A_DOUBLE::setValue(const double &value)
{
    this->value = value;
}

std::string A_DOUBLE::toString()
{
    return std::to_string(value);
}

bool A_DOUBLE::fromString(const std::string &value)
{
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

Abstract *A_DOUBLE::protectedCopy()
{
    A_DOUBLE * var = new A_DOUBLE;
    if (var) *var = this->value;
    return var;
}
