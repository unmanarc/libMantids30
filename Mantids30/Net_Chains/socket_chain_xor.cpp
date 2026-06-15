#include "socket_chain_xor.h"

#include <cstddef>
#include <cstring>
#include <vector>

using namespace Mantids30::Network::Sockets::ChainProtocols;

Socket_Chain_XOR::Socket_Chain_XOR()
{
    setXorByte(0x77);
}

ssize_t Socket_Chain_XOR::partialRead(void *data, const size_t &datalen)
{
    if (datalen == 0)
    {
        return 0;
    }

    ssize_t r = Mantids30::Network::Sockets::Socket::partialRead(data, datalen);

    if (r <= 0)
    {
        return r;
    }

    std::vector<char> datacp = getXorCopy(data, static_cast<size_t>(r));

    if (datacp.empty())
    {
        return 0;
    }

    std::memcpy(data, datacp.data(), static_cast<size_t>(r));

    return r;
}

ssize_t Socket_Chain_XOR::partialWrite(const void *data, const size_t &datalen)
{
    if (datalen == 0)
    {
        return 0;
    }

    std::vector<char> datacp = getXorCopy(data, datalen);

    if (datacp.empty())
    {
        return 0;
    }

    return Socket::partialWrite(datacp.data(), datacp.size());
}

char Socket_Chain_XOR::getXorByte() const
{
    return m_xorByte;
}

void Socket_Chain_XOR::setXorByte(char value)
{
    m_xorByte = value;
}

std::vector<char> Socket_Chain_XOR::getXorCopy(const void *data, const size_t &datalen)
{
    std::vector<char> datacp(datalen);

    const char *src = static_cast<const char *>(data);

    for (size_t i = 0; i < datalen; ++i)
    {
        datacp[i] = src[i] ^ m_xorByte;
    }

    return datacp;
}
