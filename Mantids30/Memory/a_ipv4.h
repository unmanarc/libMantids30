#pragma once

#include "a_var.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30::Memory::Abstract {

class IPV4: public Var
{
public:
    IPV4();
    IPV4(const uint32_t & value, const uint8_t & cidrMask=32);
    IPV4(const in_addr & value, const uint8_t & cidrMask=32);
    IPV4(const std::string & value);

    IPV4& operator=(const in_addr & value)
    {
        setValue(value,32);
        return *this;
    }

    in_addr getValue();
    uint8_t getCidrMask();

    bool setValue(const in_addr & value, const uint8_t & cidrMask);

    void * getDirectMemory() override { return &m_value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    bool matchRange( const char * value );
    bool matchRange( const in_addr& value );

    static bool _matchRange( const char * haystack, const in_addr& needle );
    static bool _matchRange( const char * haystack, const char * needle );
    static bool _matchRange( const in_addr& haystack, uint8_t cidr, const in_addr& needle );
    static uint8_t _toCIDRMask(const in_addr& value);
    static in_addr _fromCIDRMask(const uint8_t &value, bool * ok = nullptr);
    static std::string _toString(const in_addr& value, const uint8_t &cidrMask = 32);
    static std::string _toString(const uint32_t & value, const uint8_t &cidrMask = 32);
    static in_addr _fromString(const std::string &value, bool * ok = nullptr);

    static std::pair<in_addr, uint8_t> _fromStringWithNetmask( const std::string &value, bool * ok = nullptr );

    json toJSON() override;
    bool fromJSON(const json & value) override;


protected:
    std::shared_ptr<Var> protectedCopy() override;

private:
    static uint64_t u64pow(uint32_t base, uint32_t exponent);

    in_addr m_value = {0};
    uint8_t m_cidrMask = 32;
    Threads::Sync::Mutex_Shared m_mutex;

};

}

