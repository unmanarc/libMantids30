#include "a_ipv6.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <string.h>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Abstract;

IPV6::IPV6()
{
    memset(&value, 0, sizeof(value));
    setVarType(TYPE_IPV6);
}


IPV6::IPV6(const in6_addr &value)
{
    setVarType(TYPE_IPV6);
    memset(&this->value, 0, sizeof(value));
    setValue(value);
}

IPV6::IPV6(const std::string &value)
{
    setVarType(TYPE_IPV6);
    memset(&this->value, 0, sizeof(value));
    fromString(value);
}

in6_addr IPV6::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool IPV6::setValue(const in6_addr &value)
{
    Threads::Sync::Lock_RW lock(mutex);
#ifndef WIN32
    this->value.__in6_u = value.__in6_u;
#else
    this->value.u = value.u;
#endif
    return true;
}

std::string IPV6::toString()
{
    in6_addr xvalue = getValue();
    char cIpSource[INET6_ADDRSTRLEN+2]="";
    inet_ntop(AF_INET6, &xvalue ,cIpSource, INET6_ADDRSTRLEN+2);
    return std::string(cIpSource);
}

bool IPV6::fromString(const std::string &value)
{
    if (value.empty())
    {
        in6_addr dfl;
        memset(&dfl,0,sizeof(dfl));
        setValue(dfl);
        return true;
    }

    in6_addr xvalue;
    bool r = inet_pton(AF_INET6, value.c_str(), &xvalue)==1;
    if (r == false) return false;
    setValue(xvalue);
    return true;
}

Var *IPV6::protectedCopy()
{
    IPV6 * var = new IPV6;
    if (var) *var = getValue();
    return var;
}
