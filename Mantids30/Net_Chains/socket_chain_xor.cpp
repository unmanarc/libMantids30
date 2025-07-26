#include "socket_chain_xor.h"

#include <stdlib.h>
#include <string.h>

using namespace Mantids30::Network::Sockets::ChainProtocols;

Socket_Chain_XOR::Socket_Chain_XOR()
{
    setXorByte(0x77);
}

ssize_t Socket_Chain_XOR::partialRead(void *data, const size_t &datalen)
{
    if (!datalen) 
        return 0;

    ssize_t r = Mantids30::Network::Sockets::Socket::partialRead(data,datalen);
    if (r<=0) 
        return r;

    char * datacp = getXorCopy(data,r);
    if (!datacp) 
        return 0;
    memcpy(data,datacp,r);
    delete [] datacp;

    return r;
}

ssize_t Socket_Chain_XOR::partialWrite(const void *data, const size_t &datalen)
{
    if (!datalen) 
        return 0;

    char * datacp = getXorCopy(data,datalen);
    if (!datacp) 
        return 0;

    ssize_t r = Mantids30::Network::Sockets::Socket::partialWrite(datacp,datalen);
    delete [] datacp;
    return r;
}

char Socket_Chain_XOR::getXorByte() const
{
    return m_xorByte;
}

void Socket_Chain_XOR::setXorByte(char value)
{
    m_xorByte = value;
}

char *Socket_Chain_XOR::getXorCopy(const void *data, const size_t &datalen)
{
    char * datacp = new char[datalen];
    if (!datacp) 
        return nullptr;
    for (uint32_t i=0; i<datalen; i++)
        datacp[i] = ((char *)data)[i]^m_xorByte;
    return datacp;
}

