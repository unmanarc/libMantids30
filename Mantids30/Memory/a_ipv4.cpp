#include "a_ipv4.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Helpers/defines.h>
#include <mutex>

using namespace Mantids30::Memory::Abstract;

IPV4::IPV4()
{
    setVarType(Type::IPV4);
}

IPV4::IPV4(const uint32_t &value, const uint8_t &cidrMask)
{
    setVarType(Type::IPV4);
    in_addr x{};
    x.s_addr = value;
    setValue(x, cidrMask);
}

IPV4::IPV4(const std::string &value)
{
    setVarType(Type::IPV4);
    std::pair<in_addr, uint8_t> v = _fromStringWithNetmask(value);
    setValue(v.first, v.second);
}

in_addr IPV4::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

uint8_t IPV4::getCidrMask()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_cidrMask;
}

bool IPV4::setValue(const in_addr &value, const uint8_t &cidrMask)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    this->m_cidrMask = cidrMask;
    this->m_value.s_addr = value.s_addr;
    return true;
}

std::string IPV4::toString()
{
    return _toString(getValue());
}

bool IPV4::fromString(const std::string_view value)
{
    bool r = false;
    std::pair<in_addr, uint8_t> v = _fromStringWithNetmask(value, &r);
    setValue(v.first, v.second);
    return r;
}

uint64_t IPV4::u64pow(uint32_t base, uint32_t exponent)
{
    uint64_t result = base;

    if (exponent == 0)
    {
        return 1;
    }
    else
    {
        for (uint32_t ec = 1; ec < exponent; ec++)
        {
            result *= 2;
        }
        return result;
    }
}

bool IPV4::matchRange(const char *value)
{
    return matchRange(_fromString(value));
}

bool IPV4::matchRange(const in_addr &value)
{
    return _matchRange(getValue(), getCidrMask(), value);
}

bool IPV4::_matchRange(const char *haystack, const in_addr &needle)
{
    bool ok = true;

    std::pair<in_addr, uint8_t> v = _fromStringWithNetmask(haystack, &ok);
    if (!ok)
    {
        return false;
    }

    return _matchRange(v.first, v.second, needle);
}

bool IPV4::_matchRange(const char *haystack, const char *needle)
{
    bool ok = true;

    in_addr aNeedle = _fromString(needle, &ok);
    if (!ok)
    {
        return false;
    }

    return _matchRange(haystack, aNeedle);
}

bool IPV4::_matchRange(const in_addr &haystack, uint8_t cidr, const in_addr &needle)
{
    uint8_t baseNetmaskCIDR = cidr;
    if (baseNetmaskCIDR > 32)
    {
        return false;
    }

    uint32_t baseNetmask = htonl(u64pow(2, baseNetmaskCIDR) - 1);

    uint32_t cmp01 = (htonl(haystack.s_addr)) & baseNetmask;
    uint32_t cmp02 = (htonl(needle.s_addr)) & baseNetmask;

    return cmp01 == cmp02;
}

uint8_t IPV4::_toCIDRMask(const in_addr &value)
{
    std::string maskValue = _toString(value);

    if (maskValue == "255.255.255.255")
    {
        return 32;
    }
    if (maskValue == "255.255.255.254")
    {
        return 31;
    }
    if (maskValue == "255.255.255.252")
    {
        return 30;
    }
    if (maskValue == "255.255.255.248")
    {
        return 29;
    }
    if (maskValue == "255.255.255.240")
    {
        return 28;
    }
    if (maskValue == "255.255.255.224")
    {
        return 27;
    }
    if (maskValue == "255.255.255.192")
    {
        return 26;
    }
    if (maskValue == "255.255.255.128")
    {
        return 25;
    }

    if (maskValue == "255.255.255.0")
    {
        return 24;
    }
    if (maskValue == "255.255.254.0")
    {
        return 23;
    }
    if (maskValue == "255.255.252.0")
    {
        return 22;
    }
    if (maskValue == "255.255.248.0")
    {
        return 21;
    }
    if (maskValue == "255.255.240.0")
    {
        return 20;
    }
    if (maskValue == "255.255.224.0")
    {
        return 19;
    }
    if (maskValue == "255.255.192.0")
    {
        return 18;
    }
    if (maskValue == "255.255.128.0")
    {
        return 17;
    }

    if (maskValue == "255.255.0.0")
    {
        return 16;
    }
    if (maskValue == "255.254.0.0")
    {
        return 15;
    }
    if (maskValue == "255.252.0.0")
    {
        return 14;
    }
    if (maskValue == "255.248.0.0")
    {
        return 13;
    }
    if (maskValue == "255.240.0.0")
    {
        return 12;
    }
    if (maskValue == "255.224.0.0")
    {
        return 11;
    }
    if (maskValue == "255.192.0.0")
    {
        return 10;
    }
    if (maskValue == "255.128.0.0")
    {
        return 9;
    }

    if (maskValue == "255.0.0.0")
    {
        return 8;
    }
    if (maskValue == "254.0.0.0")
    {
        return 7;
    }
    if (maskValue == "252.0.0.0")
    {
        return 6;
    }
    if (maskValue == "248.0.0.0")
    {
        return 5;
    }
    if (maskValue == "240.0.0.0")
    {
        return 4;
    }
    if (maskValue == "224.0.0.0")
    {
        return 3;
    }
    if (maskValue == "192.0.0.0")
    {
        return 2;
    }
    if (maskValue == "128.0.0.0")
    {
        return 1;
    }
    if (maskValue == "0.0.0.0")
    {
        return 0;
    }

    return 255;
}

in_addr IPV4::_fromCIDRMask(const uint8_t &value, bool *ok)
{
    SET_OK_BOOLEAN(ok, value <= 32);

    if (value <= 32)
    {
        in_addr baseNetmask{};
        baseNetmask.s_addr = (u64pow(2, value) - 1);
        return baseNetmask;
    }
    return _fromString("0.0.0.0");
}

std::string IPV4::_toString(const in_addr &value, const uint8_t &cidrMask)
{
    char cIpSource[INET_ADDRSTRLEN] = "";
    inet_ntop(AF_INET, &value, cIpSource, INET_ADDRSTRLEN);
    return std::string(cIpSource) + (cidrMask == 32 ? "" : "/" + std::to_string(cidrMask));
}

std::string IPV4::_toString(const uint32_t &value, const uint8_t &cidrMask)
{
    in_addr x{};
    x.s_addr = value;
    return _toString(x, cidrMask);
}

in_addr IPV4::_fromString(std::string_view value, bool *ok)
{
    in_addr xvalue{};

    if (value.empty())
    {
        SET_OK_BOOLEAN(ok, true);

        return xvalue;
    }

    bool r = false;

    if (value.size() <= INET_ADDRSTRLEN - 1)
    {
        char buffer[INET_ADDRSTRLEN];

        memcpy(buffer, value.data(), value.size());
        buffer[value.size()] = '\0';

        r = inet_pton(AF_INET, buffer, &xvalue) == 1;
    }

    SET_OK_BOOLEAN(ok, r);

    return xvalue;
}
std::pair<in_addr, uint8_t> IPV4::_fromStringWithNetmask(std::string_view value, bool *ok)
{
    std::pair<in_addr, uint8_t> r{{}, 0};

    // Find the CIDR separator
    const size_t pos = value.find('/');

    // No netmask specified: use /32
    if (pos == std::string_view::npos)
    {
        r.first = _fromString(value, ok);
        r.second = 32;
        return r;
    }

    // Extract the netmask without copying the string
    const std::string_view netmask = value.substr(pos + 1);

    // Validate the netmask length
    if (netmask.empty() || netmask.size() > 2)
    {
        SET_OK_BOOLEAN(ok, false);
        return r;
    }

    // Convert the netmask to an integer
    unsigned int prefix = 0;
    const auto [ptr, ec] = std::from_chars(netmask.data(), netmask.data() + netmask.size(), prefix);

    // Validate the conversion and CIDR prefix range
    if (ec != std::errc{} || ptr != netmask.data() + netmask.size() || prefix > 32)
    {
        SET_OK_BOOLEAN(ok, false);
        return r;
    }

    // Parse the IPv4 address and store the validated prefix
    r.first = _fromString(value.substr(0, pos), ok);
    r.second = static_cast<uint8_t>(prefix);

    return r;
}

std::shared_ptr<Var> IPV4::protectedCopy()
{
    std::shared_ptr<IPV4> var = std::make_shared<IPV4>();
    if (var)
    {
        var->setValue(getValue(), getCidrMask());
    }
    return var;
}

boost::json::value IPV4::toJSON()
{
    if (isNull())
    {
        return boost::json::value{};
    }

    return boost::json::string(toString());
}

bool IPV4::fromJSON(const boost::json::value &value)
{
    return fromString(Helpers::JSON::ASSTRING_D(value, "0.0.0.0"));
}
