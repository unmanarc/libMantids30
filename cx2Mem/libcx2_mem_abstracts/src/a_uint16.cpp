#include "a_uint16.h"
#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Vars;

A_UINT16::A_UINT16()
{
    value = 0;
    setVarType(ABSTRACT_UINT16);
}

uint16_t A_UINT16::getValue()
{
    return value;
}

bool A_UINT16::setValue(uint16_t value)
{
    this->value = value;
    return true;
}

std::string A_UINT16::toString()
{
    return std::to_string(value);
}

bool A_UINT16::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = static_cast<uint16_t>(strtoul( value.c_str(), nullptr, 10 ));

    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_UINT16::protectedCopy()
{
    A_UINT16 * var = new A_UINT16;
    if (var) *var = this->value;
    return var;
}
