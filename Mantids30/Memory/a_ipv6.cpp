#include "a_ipv6.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <Mantids30/Threads/lock_shared.h>
#include <Mantids30/Helpers/mem.h>

using namespace Mantids30::Memory::Abstract;

IPV6::IPV6()
{
    ZeroBStruct(m_value);
    setVarType(TYPE_IPV6);
}


IPV6::IPV6(const in6_addr &value)
{
    setVarType(TYPE_IPV6);
    ZeroBStruct(this->m_value);
    setValue(value);
}

IPV6::IPV6(const std::string &value)
{
    setVarType(TYPE_IPV6);
    setValue(_fromString(value));
}

in6_addr IPV6::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool IPV6::setValue(const in6_addr &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
#ifndef _WIN32
    this->m_value.__in6_u = value.__in6_u;
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

std::shared_ptr<Var> IPV6::protectedCopy()
{
    auto var = std::make_shared<IPV6>();
    if (var) *var = getValue();
    return var;
}
