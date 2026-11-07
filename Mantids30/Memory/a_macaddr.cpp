#include "a_macaddr.h"
#include <string_view>

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Helpers/safeint.h>
#include <Mantids30/Helpers/defines.h>
#include <cstring>
#include <mutex>

using namespace Mantids30::Memory::Abstract;

MACADDR::MACADDR()
{
    setVarType(Type::MACADDR);
}

MACADDR::MACADDR(const unsigned char *macaddr)
{
    setVarType(Type::MACADDR);
    setValue(macaddr);
}

MACADDR::MACADDR(const std::string &value)
{
    setVarType(Type::MACADDR);
    _fromString(value, m_macaddr);
}

MACADDR::MACADDR(const uint64_t &value)
{
    setVarType(Type::MACADDR);
    _fromHASH(value, m_macaddr);
}

unsigned char *MACADDR::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_macaddr;
}

bool MACADDR::setValue(const unsigned char *macaddr)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    memcpy(this->m_macaddr, macaddr, sizeof(this->m_macaddr));
    return true;
}

std::string MACADDR::toString()
{
    return _toString(getValue());
}

bool MACADDR::fromString(std::string_view value)
{
    unsigned char _macaddr[ETH_ALEN];
    bool r = _fromString(value, _macaddr);
    setValue(_macaddr);
    return r;
}

bool MACADDR::_fromString(std::string_view src, unsigned char *dst)
{
    memset(dst, 0, sizeof(m_macaddr));

    if (src.size() != 17)
    {
        return false;
    }

    const char sep = src[2];

    if (sep != ':' && sep != '-')
    {
        return false;
    }

    unsigned char parsed[6];

    for (size_t i = 0; i < 6; ++i)
    {
        const size_t pos = i * 3;

        if (i > 0 && src[pos - 1] != sep)
        {
            return false;
        }

        unsigned int value = 0;

        auto [ptr, ec] = std::from_chars(src.data() + pos, src.data() + pos + 2, value, 16);

        if (ec != std::errc{} || ptr != src.data() + pos + 2)
        {
            return false;
        }

        parsed[i] = static_cast<unsigned char>(value);
    }

    memcpy(dst, parsed, sizeof(parsed));
    return true;
}
/**
 * Converts a 64-bit hash value to a 6-byte MAC address in network byte order (big-endian).
 * The highest 6 bytes of the 64-bit value are used, with the most significant byte first.
 *
 * @param src The 64-bit hash value.
 * @param dst Pointer to a 6-byte buffer for the MAC address.
 */
void MACADDR::_fromHASH(const uint64_t &src, unsigned char *dst)
{
    if (!dst)
    {
        return;
    }

    // Clear destination
    std::memset(dst, 0, ETH_ALEN);

    // Extract the top 6 bytes from the 64-bit value.
    // We shift right to get the most significant bytes first (big-endian order).
    // Byte 0: bits 56-63
    // Byte 1: bits 48-55
    // ...
    // Byte 5: bits 0-7 is NOT used; we use bits 8-15?
    // Actually, to preserve the hash value's significance, we typically take the most significant 6 bytes.

    // Let's define: dst[0] is the most significant byte of the MAC (network order).
    // We take the top 6 bytes of the 64-bit integer.

    dst[0] = static_cast<unsigned char>((src >> 56) & 0xFF);
    dst[1] = static_cast<unsigned char>((src >> 48) & 0xFF);
    dst[2] = static_cast<unsigned char>((src >> 40) & 0xFF);
    dst[3] = static_cast<unsigned char>((src >> 32) & 0xFF);
    dst[4] = static_cast<unsigned char>((src >> 24) & 0xFF);
    dst[5] = static_cast<unsigned char>((src >> 16) & 0xFF);
}

/**
 * Converts a 6-byte MAC address (in network byte order, big-endian) to a 64-bit hash.
 * The MAC address is placed into the most significant 6 bytes of the 64-bit value.
 *
 * @param value Pointer to a 6-byte MAC address.
 * @return A 64-bit hash value.
 */
uint64_t MACADDR::_toHash(const unsigned char *value)
{
    if (!value)
    {
        return 0;
    }

    // Construct uint64_t from 6 bytes, treating the first byte as most significant.
    uint64_t hash = 0;

    // We place the MAC bytes into the top 6 bytes of the 64-bit integer.
    // This matches the _fromHASH logic.

    hash |= static_cast<uint64_t>(value[0]) << 56;
    hash |= static_cast<uint64_t>(value[1]) << 48;
    hash |= static_cast<uint64_t>(value[2]) << 40;
    hash |= static_cast<uint64_t>(value[3]) << 32;
    hash |= static_cast<uint64_t>(value[4]) << 24;
    hash |= static_cast<uint64_t>(value[5]) << 16;

    return hash;
}

std::string MACADDR::_toString(const unsigned char *value)
{
    char addr[32];
    snprintf(addr, 32, "%02X:%02X:%02X:%02X:%02X:%02X", value[0], value[1], value[2], value[3], value[4], value[5]);
    return {addr};
}

uint64_t MACADDR::_fromStringToHASH(const std::string_view value, bool *ok)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    memset(_macaddr, 0, sizeof(_macaddr));
    bool okv = _fromString(value, _macaddr);
    SET_OK_BOOLEAN(ok, okv);
    return _toHash(_macaddr);
}

std::string MACADDR::_fromHASHToString(const uint64_t &value)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    *(reinterpret_cast<uint64_t *>(_macaddr)) = 0;
    _fromHASH(value, _macaddr);
    return _toString(_macaddr);
}

std::shared_ptr<Var> MACADDR::protectedCopy()
{
    std::shared_ptr<MACADDR> var = std::make_shared<MACADDR>();
    if (var)
    {
        *var = getValue();
    }
    return var;
}

boost::json::value MACADDR::toJSON()
{
    if (isNull())
    {
        return boost::json::value{};
    }

    return boost::json::string(toString());
}

bool MACADDR::fromJSON(const boost::json::value &value)
{
    return fromString(Helpers::JSON::ASSTRING_D(value, "0.0.0.0"));
}
