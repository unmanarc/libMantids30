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

    this->value = static_cast<uint8_t>(strtoul( value.c_str(), nullptr, 10 ));
    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_UINT8::protectedCopy()
{
    A_UINT8 * var = new A_UINT8;
    if (var) *var = this->value;
    return var;
}
