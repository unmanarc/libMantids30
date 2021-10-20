#include "a_ipv4.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_hlp_functions/mem.h>
#include <string.h>

using namespace CX2::Memory::Abstract;

IPV4::IPV4()
{
    value.s_addr = 0;
    setVarType(TYPE_IPV4);
}

IPV4::IPV4(const in_addr &value)
{
    setValue(value);
    setVarType(TYPE_IPV4);
}

IPV4::IPV4(const std::string &value)
{
    setVarType(TYPE_IPV4);
    setValue(_fromString(value));
}

in_addr IPV4::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

bool IPV4::setValue(const in_addr &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value.s_addr = value.s_addr;
    return true;
}

std::string IPV4::toString()
{   
    return _toString(getValue());
}

bool IPV4::fromString(const std::string &value)
{
    bool r;
    auto ipaddr = _fromString(value,&r);
    setValue(ipaddr);
    return r;
}

std::string IPV4::_toString(const in_addr &value)
{
    char cIpSource[INET_ADDRSTRLEN]="";
    inet_ntop(AF_INET, &value ,cIpSource, INET_ADDRSTRLEN);
    return std::string(cIpSource);
}

in_addr IPV4::_fromString(const std::string &value, bool *ok)
{
    in_addr xvalue;
    ZeroBStruct(xvalue);

    if (value.empty())
    {
        if (ok) *ok = true;
        return xvalue;
    }

    bool r = inet_pton(AF_INET, value.c_str(), &xvalue)==1;
    if (ok) *ok = r;

    return xvalue;
}

Var *IPV4::protectedCopy()
{
    IPV4 * var = new IPV4;
    if (var) *var = getValue();
    return var;
}
