#include "abstractvar.h"
#include "Mantids29/Helpers/mem.h"
#include "a_var.h"
#include <cstdint>
#include <netinet/in.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <in6addr.h>
#else
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include "w32compat.h"
#else
#include <arpa/inet.h>
#endif


using namespace Mantids29::Memory::Abstract;
#include <Mantids29/Threads/lock_shared.h>

template<typename T>
AbstractVar<T>::AbstractVar() : value(0)
{
    varType = TYPE_NULL;
}
/*
template<>
AbstractVar<in_addr>::AbstractVar()
{
    value.s_addr = 0;
    varType = TYPE_BOOL;
}

template<>
AbstractVar<in6_addr>::AbstractVar()
{
    ZeroBStructNS(value);
    varType = TYPE_BOOL;
}
*/
template<>
AbstractVar<bool>::AbstractVar() : value(0)
{
    varType = TYPE_BOOL;
}

template<>
AbstractVar<int8_t>::AbstractVar() : value(0)
{
    varType = TYPE_INT8;
}

template<>
AbstractVar<int16_t>::AbstractVar() : value(0)
{
    varType = TYPE_INT16;
}

template<>
AbstractVar<int32_t>::AbstractVar() : value(0)
{
    varType = TYPE_INT32;
}

template<>
AbstractVar<int64_t>::AbstractVar() : value(0)
{
    varType = TYPE_INT64;
}

template<>
AbstractVar<uint8_t>::AbstractVar() : value(0)
{
    varType = TYPE_UINT8;
}

template<>
AbstractVar<uint16_t>::AbstractVar() : value(0)
{
    varType = TYPE_UINT16;
}

template<>
AbstractVar<uint32_t>::AbstractVar() : value(0)
{
    varType = TYPE_UINT32;
}

template<>
AbstractVar<uint64_t>::AbstractVar() : value(0)
{
    varType = TYPE_UINT64;
}

template<>
AbstractVar<double>::AbstractVar() : value(0)
{
    varType = TYPE_DOUBLE;
}

template<>
AbstractVar<std::string>::AbstractVar() : value("")
{
    varType = TYPE_STRING;
}

template<typename T>
bool AbstractVar<T>::setValue(const T &value)
{
    Threads::Sync::Lock_RW lock(mutex);
    this->value = value;
    return true;
}

template<typename T>
void *AbstractVar<T>::getDirectMemory()
{
    return &value;
}

template<typename T>
std::string AbstractVar<T>::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return std::to_string(value);
}
/*
template<>
std::string AbstractVar<in6_addr>::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    char cIpSource[INET6_ADDRSTRLEN]="";
    inet_ntop(AF_INET6, &value ,cIpSource, INET6_ADDRSTRLEN);
    return std::string(cIpSource);
}*/
/*
template<>
std::string AbstractVar<in_addr>::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    char cIpSource[INET_ADDRSTRLEN]="";
    inet_ntop(AF_INET, &value ,cIpSource, INET_ADDRSTRLEN);
    return std::string(cIpSource) + (cidrMask==32?"":"/" + std::to_string(cidrMask));
}
*/
template<>
std::string AbstractVar<std::string>::toString()
{
    Threads::Sync::Lock_RD lock(mutex);
    return value;
}

template<typename T>
bool AbstractVar<T>::fromString(const std::string &value)
{
    try
    {
        Threads::Sync::Lock_RW lock(mutex);
        this->value = std::stoll(value);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

Base *Base::copy()
{
    Base * var = protectedCopy();
    if (var) var->varType= this->varType;
    return var;
}

Base *Base::makeAbstract(Type type, const std::string &defValue)
{
    Base * v = nullptr;
    switch (type)
    {
    case TYPE_NULL:
        v=new Base;
        break;
    case TYPE_BOOL:
        v=new AbstractVar<bool>;
        break;
    case TYPE_INT8:
        v=new AbstractVar<int8_t>;
        break;
    case TYPE_INT16:
        v=new AbstractVar<int16_t>;
        break;
    case TYPE_INT32:
        v=new AbstractVar<int32_t>;
        break;
    case TYPE_INT64:
        v=new AbstractVar<int64_t>;
        break;
    case TYPE_UINT8:
        v=new AbstractVar<uint8_t>;
        break;
    case TYPE_UINT16:
        v=new AbstractVar<uint16_t>;
        break;
    case TYPE_UINT32:
        v=new AbstractVar<uint32_t>;
        break;
    case TYPE_UINT64:
        v=new AbstractVar<uint64_t>;
        break;
    case TYPE_DOUBLE:
        v=new AbstractVar<double>;
        break;
/*    case TYPE_DATETIME:
        v=new DATETIME;
        break;
    case TYPE_BIN:
        v=new BINARY;
        break;*/
    case TYPE_STRING:
        v=new AbstractVar<std::string>;
        break;
/*    case TYPE_STRINGLIST:
        v=new STRINGLIST;
        break;*/
    /*case TYPE_IPV4:
        v=new AbstractVar<in_addr>;
        break;*/
/*    case TYPE_MACADDR:
        v=new AbstractVar<float>;
        break;*/
    /*case TYPE_IPV6:
        v=new AbstractVar<in6_addr>;
        break;*/
/*    case TYPE_VARCHAR:
        v=new VARCHAR(defValue.size()+1024);
        break;*/
    default:
        break;
    }

    if (v) v->fromString(defValue);

    return v;
}
