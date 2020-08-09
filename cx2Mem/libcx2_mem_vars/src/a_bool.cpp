#include "a_bool.h"
#include <stdexcept>      // std::invalid_argument

using namespace CX2::Memory::Vars;

A_BOOL::A_BOOL()
{
    value = false;
    setVarType(ABSTRACT_BOOL);

}

bool A_BOOL::getValue()
{
    return value;
}

bool A_BOOL::setValue(bool value)
{
    this->value = value;
    return true;
}

std::string A_BOOL::toString()
{
    return value?"true":"false";
}

bool A_BOOL::fromString(const std::string &value)
{
    if (value == "true" || value == "TRUE" || value == "1" || value == "t" || value == "T") this->value = true;
    else this->value = false;
    return true;
}

Abstract *A_BOOL::protectedCopy()
{
    A_BOOL * var = new A_BOOL;
    if (var) *var = this->value;
    return var;
}
