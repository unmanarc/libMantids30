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

    this->value = static_cast<uint32_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_UINT32::protectedCopy()
{
    A_UINT32 * var = new A_UINT32;
    if (var) *var = this->value;
    return var;
}
