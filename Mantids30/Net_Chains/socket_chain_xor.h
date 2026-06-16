#pragma once

#include "socket_chain_protocolbase.h"
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <vector>

namespace Mantids30::Network::Sockets::ChainProtocols {

/**
 * @brief The SocketChainXOR class
 *        Proof of concept of socket transformation, don't use for security applications.
 */
class Socket_Chain_XOR : public Mantids30::Network::Sockets::Socket_Stream, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_XOR();

    // Overwritten functions:
    ssize_t partialRead(void *data, const size_t &datalen) override;
    ssize_t partialWrite(const void *data, const size_t &datalen) override;

    // Private functions:
    char getXorByte() const;
    void setXorByte(char value);

protected:
    void *getThis() override { return this; }

private:
    std::vector<char> getXorCopy(const void *data, const size_t &datalen) const;
    char m_xorByte;
};

} // namespace Mantids30::Network::Sockets::ChainProtocols
