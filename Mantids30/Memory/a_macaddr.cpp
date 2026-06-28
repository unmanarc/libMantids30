#include "a_macaddr.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Helpers/safeint.h>
#include <Mantids30/Threads/lock_shared.h>
#include <cstring>

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
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_macaddr;
}

bool MACADDR::setValue(const unsigned char *macaddr)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    memcpy(this->m_macaddr, macaddr, sizeof(this->m_macaddr));
    return true;
}

std::string MACADDR::toString()
{
    return _toString(getValue());
}

bool MACADDR::fromString(const std::string &value)
{
    unsigned char _macaddr[ETH_ALEN];
    bool r = _fromString(value, _macaddr);
    setValue(_macaddr);
    return r;
}

bool MACADDR::_fromString(const std::string &src, unsigned char *dst)
{
    bool r = false;

    memset(dst, 0, sizeof(m_macaddr));
    unsigned int a, b, c, d, e, f;
    if (sscanf(src.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", &a, &b, &c, &d, &e, &f) == 6 || sscanf(src.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &a, &b, &c, &d, &e, &f) == 6
        || sscanf(src.c_str(), "%02X-%02X-%02X-%02X-%02X-%02X", &a, &b, &c, &d, &e, &f) == 6 || sscanf(src.c_str(), "%02x-%02x-%02x-%02x-%02x-%02x", &a, &b, &c, &d, &e, &f) == 6)
    {
        dst[0] = safe_cast_or<unsigned char>(a, 0);
        dst[1] = safe_cast_or<unsigned char>(b, 0);
        dst[2] = safe_cast_or<unsigned char>(c, 0);
        dst[3] = safe_cast_or<unsigned char>(d, 0);
        dst[4] = safe_cast_or<unsigned char>(e, 0);
        dst[5] = safe_cast_or<unsigned char>(f, 0);
        r = true;
    }

    return r;
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

uint64_t MACADDR::_fromStringToHASH(const std::string &value, bool *ok)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    memset(_macaddr, 0, sizeof(_macaddr));
    bool okv = _fromString(value, _macaddr);
    if (ok)
    {
        *ok = okv;
    }
    return _toHash(_macaddr);
}

std::string MACADDR::_fromHASHToString(const uint64_t &value)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    *((uint64_t *) _macaddr) = 0;
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

json MACADDR::toJSON()
{
    if (isNull())
    {
        return Json::nullValue;
    }

    return toString();
}

bool MACADDR::fromJSON(const json &value)
{
    return fromString(Helpers::JSON::ASSTRING_D(value, "0.0.0.0"));
}
