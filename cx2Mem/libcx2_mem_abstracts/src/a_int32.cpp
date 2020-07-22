#include "a_int32.h"

using namespace CX2::Memory::Vars;


A_INT32::A_INT32()
{
    value = 0;
    setVarType(ABSTRACT_INT32);
}

int32_t A_INT32::getValue()
{
    return value;
}

bool A_INT32::setValue(int32_t value)
{
    this->value = value;
    return true;
}

std::string A_INT32::toString()
{
    return std::to_string(value);

}

bool A_INT32::fromString(const std::string &value)
{
    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtol(value.c_str(),nullptr,10);
    if (value!="0" && this->value==0) return false;

    return true;
}

Abstract *A_INT32::protectedCopy()
{
    A_INT32 * var = new A_INT32;
    if (var) *var = this->value;
    return var;
}
