#include "a_uint32.h"
#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Vars;

A_UINT32::A_UINT32()
{
    value = 0;
    setVarType(ABSTRACT_UINT32);
}

uint32_t A_UINT32::getValue()
{
    return value;
}

bool A_UINT32::setValue(uint32_t value)
{
    this->value = value;
    return true;
}

std::string A_UINT32::toString()
{
    return std::to_string(value);

}

bool A_UINT32::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    size_t pos ;
    try
    {
        this->value = static_cast<uint32_t>(std::stoul( value, &pos, 10 ));
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

Abstract *A_UINT32::protectedCopy()
{
    A_UINT32 * var = new A_UINT32;
    if (var) *var = this->value;
    return var;
}
