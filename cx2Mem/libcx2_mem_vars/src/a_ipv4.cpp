#include "a_ipv4.h"
#include <arpa/inet.h>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Vars;

A_IPV4::A_IPV4()
{
    value.s_addr = 0;
    setVarType(ABSTRACT_IPV4);
}

A_IPV4::~A_IPV4()
{
}

in_addr A_IPV4::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool A_IPV4::setValue(const in_addr &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value.s_addr = value.s_addr;
    return true;
}

std::string A_IPV4::toString()
{
    in_addr xvalue = getValue();
    char cIpSource[INET_ADDRSTRLEN+2]="";
    inet_ntop(AF_INET, &xvalue ,cIpSource, INET_ADDRSTRLEN+2);
    return std::string(cIpSource);
}

bool A_IPV4::fromString(const std::string &value)
{
    if (value.empty())
    {
        in_addr dfl;
        dfl.s_addr = 0;
        setValue(dfl);
        return true;
    }

    in_addr xvalue;
    bool r = inet_pton(AF_INET, value.c_str(), &xvalue)==1;
    if (r == false) return false;
    setValue(xvalue);
    return true;
}

Abstract *A_IPV4::protectedCopy()
{
    A_IPV4 * var = new A_IPV4;
    if (var) *var = getValue();
    return var;
}
