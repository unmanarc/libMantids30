#ifndef SOCKETCHAINXOR_H
#define SOCKETCHAINXOR_H

#include <mdz_net_sockets/streamsocket.h>
#include "socketchainbase.h"

namespace Mantids { namespace Network { namespace Chains { namespace Protocols {

/**
 * @brief The SocketChainXOR class
 *        Proof of concept of socket transformation, don't use for security applications.
 */
class SocketChain_XOR : public Mantids::Network::Streams::StreamSocket, public SocketChainBase
{
public:
    SocketChain_XOR();

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

#endif // SOCKETCHAINXOR_H
