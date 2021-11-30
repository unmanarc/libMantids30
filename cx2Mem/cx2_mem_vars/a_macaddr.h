#ifndef A_MACADDR_H
#define A_MACADDR_H

#include "a_var.h"
#include "endian2.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <cx2_thr_mutex/mutex_shared.h>

#define ETH_ALEN 6


namespace CX2 { namespace Memory { namespace Abstract {

class MACADDR: public Var
{
public:
    MACADDR();
    MACADDR(const unsigned char * macaddr);
    MACADDR(const std::string & value);
    MACADDR(const uint64_t & value);

    MACADDR& operator=(const unsigned char * value)
    {
        setValue(value);
        return *this;
    }

    unsigned char *getValue();
    bool setValue(const unsigned char * macaddr);

    void * getDirectMemory() override { return &macaddr; }

    std::string toString() override;
    bool fromString(const std::string & value) override;

    static bool _fromString(const std::string & src,  unsigned char *dst);
    static void _fromHASH(const uint64_t &src,  unsigned char *dst);

    static std::string _toString(const unsigned char *value);
    static uint64_t _toHash(const unsigned char *value);
    static uint64_t _fromStringToHASH(const std::string &value, bool * ok = nullptr);
    static std::string _fromHASHToString(const uint64_t &value);

protected:
    Var * protectedCopy() override;

private:
    unsigned char macaddr[ETH_ALEN];
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_MACADDR_H
