#include "a_uint8.h"
#include <stdexcept>      // std::invalid_argument

using namespace CX2::Memory::Vars;

A_UINT8::A_UINT8()
{
    value = 0;
    setVarType(ABSTRACT_UINT8);
}

uint8_t A_UINT8::getValue()
{
    return value;
}

bool A_UINT8::setValue(uint8_t value)
{
    this->value = value;
    return true;
}

std::string A_UINT8::toString()
{
    return std::to_string(value);
}

bool A_UINT8::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    size_t pos ;
    try
    {
        this->value = static_cast<uint8_t>(std::stoul( value, &pos, 10 ));
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

Abstract *A_UINT8::protectedCopy()
{
    A_UINT8 * var = new A_UINT8;
    if (var) *var = this->value;
    return var;
}
