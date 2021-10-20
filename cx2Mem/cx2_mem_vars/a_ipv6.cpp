#include "a_ipv6.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <string.h>
#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_hlp_functions/mem.h>

using namespace CX2::Memory::Abstract;

IPV6::IPV6()
{
    ZeroBStruct(value);
    setVarType(TYPE_IPV6);
}


IPV6::IPV6(const in6_addr &value)
{
    setVarType(TYPE_IPV6);
    ZeroBStruct(this->value);
    setValue(value);
}

IPV6::IPV6(const std::string &value)
{
    setVarType(TYPE_IPV6);
    setValue(_fromString(value));
}

in6_addr IPV6::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool IPV6::setValue(const in6_addr &value)
{
    Threads::Sync::Lock_RW lock(mutex);
#ifndef _WIN32
    this->value.__in6_u = value.__in6_u;
#else
    this->value.u = value.u;
#endif
    return true;
}

std::string IPV6::toString()
{
    return _toString(getValue());
}

bool IPV6::fromString(const std::string &value)
{
    bool r;
    auto ipaddr = _fromString(value,&r);
    setValue(ipaddr);
    return r;
}

std::string IPV6::_toString(const in6_addr &value)
{
    char cIpSource[INET6_ADDRSTRLEN]="";
    inet_ntop(AF_INET6, &value ,cIpSource, INET6_ADDRSTRLEN);
    return std::string(cIpSource);
}

in6_addr IPV6::_fromString(const std::string &value, bool *ok)
{
    in6_addr xvalue;
    ZeroBStruct(xvalue);

    if (value.empty())
    {
        if (ok) *ok = true;
        return xvalue;
    }

    bool r = inet_pton(AF_INET6, value.c_str(), &xvalue)==1;
    if (ok) *ok = r;

    return xvalue;
}

Var *IPV6::protectedCopy()
{
    IPV6 * var = new IPV6;
    if (var) *var = getValue();
    return var;
}
