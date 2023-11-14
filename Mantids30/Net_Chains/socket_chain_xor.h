#pragma once

#include <Mantids30/Net_Sockets/socket_stream_base.h>
#include "socket_chain_protocolbase.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace ChainProtocols {

/**
 * @brief The SocketChainXOR class
 *        Proof of concept of socket transformation, don't use for security applications.
 */
class Socket_Chain_XOR : public Mantids30::Network::Sockets::Socket_Stream_Base, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_XOR();

    // Overwritten functions:
    int partialRead(void * data, const uint32_t & datalen);
    int partialWrite(const void * data, const uint32_t & datalen);

    // Private functions:
    char getXorByte() const;
    void setXorByte(char value);

protected:
    void * getThis() { return this; }

private:
    char * getXorCopy(const void *data, const uint32_t & datalen);
    char xorByte;
};


}}}}

