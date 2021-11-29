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

uint64_t IPV4::u64pow(uint32_t base, uint32_t exponent)
{
    uint64_t result=base;

    if (exponent==0)
        return 1;
    else
    {
        for (uint32_t ec = 1; ec < exponent; ec++)
            result*=2;
        return result;
    }
}

bool IPV4::matchRange(const char * value)
{
    return matchRange(_fromString(value));
}

bool IPV4::matchRange(const in_addr &value)
{
    return _matchRange(getValue(),getCidrMask(),value);
}

bool IPV4::_matchRange(const char *haystack, const in_addr &needle)
{
    bool ok = true;

    auto v = _fromStringWithNetmask(haystack,&ok);
    if (!ok) return false;

    return _matchRange( v.first,v.second,needle );
}

bool IPV4::_matchRange(const char *haystack, const char *needle)
{
    bool ok = true;

    auto aNeedle = _fromString(needle,&ok);
    if (!ok) return false;

    return _matchRange(haystack,aNeedle);
}

bool IPV4::_matchRange(const in_addr &haystack, uint8_t cidr, const in_addr &needle)
{
    auto baseNetmaskCIDR = cidr;
    if (baseNetmaskCIDR>32)
        return false;

    uint32_t baseNetmask = htonl(u64pow(2,baseNetmaskCIDR)-1);

    auto cmp01=(htonl(haystack.s_addr))&baseNetmask;
    auto cmp02=(htonl(needle.s_addr))&baseNetmask;

    return  cmp01 == cmp02;
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
    {
        if (value>32)
            *ok = false;
        else
            *ok=true;
    }

    if (value<=32)
    {
        in_addr baseNetmask;
        baseNetmask.s_addr = (u64pow(2,value)-1);
        return baseNetmask;
    }
    return  _fromString("0.0.0.0");
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
    if (var)
        var->setValue( getValue(),getCidrMask() );
    return var;
}
