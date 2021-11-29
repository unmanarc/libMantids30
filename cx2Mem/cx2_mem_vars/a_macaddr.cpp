#include "a_macaddr.h"

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif

#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_hlp_functions/mem.h>
#include <string.h>


using namespace CX2::Memory::Abstract;

MACADDR::MACADDR()
{
    setVarType(TYPE_MACADDR);
    memset(macaddr,0,sizeof (macaddr));
}

MACADDR::MACADDR(const unsigned char * macaddr)
{
    setVarType(TYPE_MACADDR);
    setValue(macaddr);
}

MACADDR::MACADDR(const std::string &value)
{
    setVarType(TYPE_MACADDR);
    _fromString(value,macaddr);
}

MACADDR::MACADDR(const uint64_t &value)
{
    setVarType(TYPE_MACADDR);
    _fromHASH(value,macaddr);
}

unsigned char *MACADDR::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);
    return macaddr;
}

bool MACADDR::setValue(const unsigned char * macaddr)
{
    Threads::Sync::Lock_RW lock(mutex);
    memcpy(this->macaddr,macaddr,sizeof (this->macaddr));
    return true;
}

std::string MACADDR::toString()
{   
    return _toString(getValue());
}

bool MACADDR::fromString(const std::string &value)
{
    unsigned char _macaddr[ETH_ALEN];
    bool r = _fromString(value,_macaddr);
    setValue(_macaddr);
    return r;
}

bool MACADDR::_fromString(const std::string &src, unsigned char *dst)
{
    bool r = false;

    memset(dst,0,sizeof (macaddr));
    unsigned int a,b,c,d,e,f;
    if (    sscanf(src.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",&a,&b,&c,&d,&e,&f)==6
        ||  sscanf(src.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",&a,&b,&c,&d,&e,&f)==6
        ||  sscanf(src.c_str(), "%02X-%02X-%02X-%02X-%02X-%02X",&a,&b,&c,&d,&e,&f)==6
        ||  sscanf(src.c_str(), "%02x-%02x-%02x-%02x-%02x-%02x",&a,&b,&c,&d,&e,&f)==6
            )
    {
        dst[0] = (unsigned char)a;
        dst[1] = (unsigned char)b;
        dst[2] = (unsigned char)c;
        dst[3] = (unsigned char)d;
        dst[4] = (unsigned char)e;
        dst[5] = (unsigned char)f;
        r=true;
    }

    return r;
}

void MACADDR::_fromHASH(const uint64_t &src, unsigned char *dst)
{
    memset(dst,0,sizeof(ETH_ALEN));
    *((uint64_t *)dst) = htonll(src);
}

std::string MACADDR::_toString(const unsigned char *value)
{
    char addr[32];
    snprintf(addr,32,"%02X:%02X:%02X:%02X:%02X:%02X",value[0],value[1],value[2],value[3],value[4],value[5]);
    return std::string(addr);
}

uint64_t MACADDR::_fromStringToHASH(const std::string &value, bool *ok)
{
    unsigned char _macaddr[sizeof(uint64_t)];
    bool okv = _fromString(value, _macaddr);
    if (ok ) *ok = okv;
    return ntohll(*((uint64_t *)_macaddr));
}

std::string MACADDR::_fromHASHToString(const uint64_t &value)
{
    unsigned char _macaddr[sizeof(uint64_t)];
   *((uint64_t *)_macaddr) = 0;
    _fromHASH(value,_macaddr);
    return _toString(_macaddr);
}

Var *MACADDR::protectedCopy()
{
    MACADDR * var = new MACADDR;
    if (var) *var = getValue();
    return var;
}
