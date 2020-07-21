#include "a_int64.h"
#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Vars;

A_INT64::A_INT64()
{
    value = 0;
    setVarType(ABSTRACT_INT64);
}

int64_t A_INT64::getValue()
{
    return value;
}

bool A_INT64::setValue(const int64_t &value)
{
    this->value = value;
    return true;
}

std::string A_INT64::toString()
{
    return std::to_string(value);

}

bool A_INT64::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    size_t pos ;
    try
    {
        this->value = std::stoll( value, &pos, 10 ) ;
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

Abstract *A_INT64::protectedCopy()
{
    A_INT64 * var = new A_INT64;
    if (var) *var = this->value;
    return var;
}
