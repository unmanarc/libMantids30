#include "a_macaddr.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Helpers/safeint.h>
#include <Mantids30/Threads/lock_shared.h>
#include <string.h>

using namespace Mantids30::Memory::Abstract;

MACADDR::MACADDR()
{
    setVarType(TYPE_MACADDR);
    memset(m_macaddr, 0, sizeof(m_macaddr));
}

MACADDR::MACADDR(const unsigned char *macaddr)
{
    setVarType(TYPE_MACADDR);
    setValue(macaddr);
}

MACADDR::MACADDR(const std::string &value)
{
    setVarType(TYPE_MACADDR);
    _fromString(value, m_macaddr);
}

MACADDR::MACADDR(const uint64_t &value)
{
    setVarType(TYPE_MACADDR);
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
        dst[0] = unsafeStaticCast<unsigned char, unsigned int>(a);
        dst[1] = unsafeStaticCast<unsigned char, unsigned int>(b);
        dst[2] = unsafeStaticCast<unsigned char, unsigned int>(c);
        dst[3] = unsafeStaticCast<unsigned char, unsigned int>(d);
        dst[4] = unsafeStaticCast<unsigned char, unsigned int>(e);
        dst[5] = unsafeStaticCast<unsigned char, unsigned int>(f);
        r = true;
    }

    return r;
}

void MACADDR::_fromHASH(const uint64_t &src, unsigned char *dst)
{
    memset(dst, 0, sizeof(ETH_ALEN));
    char dst2[sizeof(uint64_t)];
    *((uint64_t *) dst2) = htonll(src);
    memcpy(dst, dst2, ETH_ALEN);
}

std::string MACADDR::_toString(const unsigned char *value)
{
    char addr[32];
    snprintf(addr, 32, "%02X:%02X:%02X:%02X:%02X:%02X", value[0], value[1], value[2], value[3], value[4], value[5]);
    return std::string(addr);
}

uint64_t MACADDR::_toHash(const unsigned char *value)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    *((uint64_t *) _macaddr) = 0;
    memcpy(_macaddr, value, 6);
    return ntohll(*((uint64_t *) _macaddr));
}

uint64_t MACADDR::_fromStringToHASH(const std::string &value, bool *ok)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    memset(_macaddr, 0, sizeof(_macaddr));
    bool okv = _fromString(value, _macaddr);
    if (ok)
        *ok = okv;
    return ntohll(*((uint64_t *) _macaddr));
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
    auto var = std::make_shared<MACADDR>();
    if (var)
        *var = getValue();
    return var;
}
json MACADDR::toJSON()
{
    if (getIsNull())
        return Json::nullValue;

    return toString();
}

bool MACADDR::fromJSON(const json &value)
{
    return fromString(JSON_ASSTRING_D(value, "0.0.0.0"));
}
