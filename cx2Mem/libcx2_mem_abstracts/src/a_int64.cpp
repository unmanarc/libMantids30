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

    this->value = strtoll(value.c_str(),nullptr,10);
    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_INT64::protectedCopy()
{
    A_INT64 * var = new A_INT64;
    if (var) *var = this->value;
    return var;
}
