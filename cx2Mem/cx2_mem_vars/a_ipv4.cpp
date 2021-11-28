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
    cidrMask = 32;
    value.s_addr = 0;
    setVarType(TYPE_IPV4);
}

IPV4::IPV4(const in_addr &value, const uint8_t &cidrMask)
{
    setValue(value,cidrMask);
    setVarType(TYPE_IPV4);
}

IPV4::IPV4(const std::string &value)
{
    setVarType(TYPE_IPV4);
    auto v = _fromStringWithNetmask(value);
    setValue(v.first,v.second);
}

in_addr IPV4::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

uint8_t IPV4::getCidrMask()
{
    Threads::Sync::Lock_RD lock(mutex);
    return cidrMask;
}

bool IPV4::setValue(const in_addr &value, const uint8_t &cidrMask)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->cidrMask = cidrMask;
    this->value.s_addr = value.s_addr;
    return true;
}

std::string IPV4::toString()
{   
    return _toString(getValue());
}

bool IPV4::fromString(const std::string &value)
{
    bool r = false;
    auto v = _fromStringWithNetmask(value, &r);
    setValue(v.first,v.second);
    return r;
}

uint8_t IPV4::_toCIDRMask(const in_addr &value)
{
    std::string maskValue = _toString(value);

    if (maskValue == "255.255.255.255") return 32;
    if (maskValue == "255.255.255.254") return 31;
    if (maskValue == "255.255.255.252") return 30;
    if (maskValue == "255.255.255.248") return 29;
    if (maskValue == "255.255.255.240") return 28;
    if (maskValue == "255.255.255.224") return 27;
    if (maskValue == "255.255.255.192") return 26;
    if (maskValue == "255.255.255.128") return 25;

    if (maskValue == "255.255.255.0") return 24;
    if (maskValue == "255.255.254.0") return 23;
    if (maskValue == "255.255.252.0") return 22;
    if (maskValue == "255.255.248.0") return 21;
    if (maskValue == "255.255.240.0") return 20;
    if (maskValue == "255.255.224.0") return 19;
    if (maskValue == "255.255.192.0") return 18;
    if (maskValue == "255.255.128.0") return 17;

    if (maskValue == "255.255.0.0") return 16;
    if (maskValue == "255.254.0.0") return 15;
    if (maskValue == "255.252.0.0") return 14;
    if (maskValue == "255.248.0.0") return 13;
    if (maskValue == "255.240.0.0") return 12;
    if (maskValue == "255.224.0.0") return 11;
    if (maskValue == "255.192.0.0") return 10;
    if (maskValue == "255.128.0.0") return 9;

    if (maskValue == "255.0.0.0") return 8;
    if (maskValue == "254.0.0.0") return 7;
    if (maskValue == "252.0.0.0") return 6;
    if (maskValue == "248.0.0.0") return 5;
    if (maskValue == "240.0.0.0") return 4;
    if (maskValue == "224.0.0.0") return 3;
    if (maskValue == "192.0.0.0") return 2;
    if (maskValue == "128.0.0.0") return 1;
    if (maskValue == "0.0.0.0") return 0;

    return 255;
}

in_addr IPV4::_fromCIDRMask(const uint8_t &value, bool *ok)
{
    if (ok)
        *ok = true;
    switch (value)
    {
    case 32: return _fromString("255.255.255.255");
    case 31: return _fromString("255.255.255.254");
    case 30: return _fromString("255.255.255.252");
    case 29: return _fromString("255.255.255.248");
    case 28: return _fromString("255.255.255.240");
    case 27: return _fromString("255.255.255.224");
    case 26: return _fromString("255.255.255.192");
    case 25: return _fromString("255.255.255.128");

    case 24: return _fromString("255.255.255.0");
    case 23: return _fromString("255.255.254.0");
    case 22: return _fromString("255.255.252.0");
    case 21: return _fromString("255.255.248.0");
    case 20: return _fromString("255.255.240.0");
    case 19: return _fromString("255.255.224.0");
    case 18: return _fromString("255.255.192.0");
    case 17: return _fromString("255.255.128.0");

    case 16: return _fromString("255.255.0.0");
    case 15: return _fromString("255.254.0.0");
    case 14: return _fromString("255.252.0.0");
    case 13: return _fromString("255.248.0.0");
    case 12: return _fromString("255.240.0.0");
    case 11: return _fromString("255.224.0.0");
    case 10: return _fromString("255.192.0.0");
    case 9: return _fromString("255.128.0.0");

    case 8: return _fromString("255.0.0.0");
    case 7: return _fromString("254.0.0.0");
    case 6: return _fromString("252.0.0.0");
    case 5: return _fromString("248.0.0.0");
    case 4: return _fromString("240.0.0.0");
    case 3: return _fromString("224.0.0.0");
    case 2: return _fromString("192.0.0.0");
    case 1: return _fromString("128.0.0.0");
    case 0: return _fromString("0.0.0.0");

    default:
        if (ok)
            *ok = false;
        return  _fromString("0.0.0.0");
    }
}

std::string IPV4::_toString(const in_addr &value, const uint8_t & cidrMask)
{
    char cIpSource[INET_ADDRSTRLEN]="";
    inet_ntop(AF_INET, &value ,cIpSource, INET_ADDRSTRLEN);
    return std::string(cIpSource) + (cidrMask==32?"":"/" + std::to_string(cidrMask));
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

std::pair<in_addr, uint8_t> IPV4::_fromStringWithNetmask(const std::string &value, bool *ok)
{
    std::pair<in_addr, uint8_t> r;

    r.first.s_addr=0;
    r.second = 0;

    if (value.find_first_of('/') == std::string::npos )
    {
        r.second = 32;
        r.first = _fromString(value, ok);
    }
    else
    {
        std::string detectedNetmask = value.substr( value.find_first_of('/')+1 );
        if (detectedNetmask.size()!=1 && detectedNetmask.size()!=2)
        {
            if (ok)
                *ok = false;
        }
        else
        {
            r.second = std::strtoul(detectedNetmask.c_str(),0,10);
            if (r.second>32)
            {
                if (ok)
                    *ok = false;
                r.second = 0;
            }
            else
                r.first = _fromString(value.substr( 0,value.find_first_of('/') ),ok);
        }
    }
    return r;
}

Var *IPV4::protectedCopy()
{
    IPV4 * var = new IPV4;
    if (var) *var = getValue();
    return var;
}
