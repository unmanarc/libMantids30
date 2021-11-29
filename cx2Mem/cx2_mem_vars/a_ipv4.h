#ifndef A_IPV4_H
#define A_IPV4_H

#include "a_var.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

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

    void * getDirectMemory() override { return &value; }

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

protected:
    Var * protectedCopy() override;

private:
    static uint64_t u64pow(uint32_t base, uint32_t exponent);

    in_addr value;
    uint8_t cidrMask;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_IPV4_H
