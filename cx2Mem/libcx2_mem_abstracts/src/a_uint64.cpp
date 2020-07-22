#include "a_uint64.h"
#include <stdexcept>      // std::invalid_argument
using namespace CX2::Memory::Vars;

A_UINT64::A_UINT64()
{
    value = 0;
    setVarType(ABSTRACT_UINT64);
}

uint64_t A_UINT64::getValue()
{
    return value;
}

bool A_UINT64::setValue(const uint64_t &value)
{
    this->value = value;
    return true;
}

std::string A_UINT64::toString()
{
    return std::to_string(value);
}

bool A_UINT64::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtoull( value.c_str(), nullptr, 10 );
    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_UINT64::protectedCopy()
{
    A_UINT64 * var = new A_UINT64;
    if (var) *var = this->value;
    return var;
}
