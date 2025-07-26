#include "socket_stream_dummy.h"

using namespace Mantids30::Network::Sockets;

std::string Socket_Stream_Dummy::getPeerName() const
{
    return peerName;
}

void Socket_Stream_Dummy::setPeerName(const std::string &newPeerName)
{
    peerName = newPeerName;
}

bool Socket_Stream_Dummy::isSecure()
{
    return true;
}


bool Socket_Stream_Dummy::isConnected()
{
    return true;
}

int Socket_Stream_Dummy::shutdownSocket(
    int mode)
{
    if (mode == SHUT_RDWR)
    {
        shuttedDownRD = shuttedDownWR = true;
    }
    else if (mode == SHUT_RD)
    {
        shuttedDownRD = true;
    }
    else if (mode == SHUT_WR)
    {
        shuttedDownWR = true;
    }
    return 0;
}

ssize_t Socket_Stream_Dummy::partialRead(void *data, const size_t &datalen)
{
    auto storedDataSize = sender.size();
    if (!storedDataSize)
    {
        return 0;
    }
    else if (datalen <= storedDataSize)
    {
        // Enviar solo datalen...
        sender.copyOut(data, datalen);
        sender.displace(datalen);
        return datalen;
    }
    else
    {
        // Send all stored data and clear the container
        sender.copyOut(data, storedDataSize);
        sender.clear();
        return storedDataSize;
    }
}

ssize_t Socket_Stream_Dummy::partialWrite(const void *data, const size_t &datalen)
{
    std::optional<size_t> r = receiver.append(data, datalen);
    if (!r)
        return -1;
    else
        return *r;
}

void Socket_Stream_Dummy::setRemotePairOverride(
    const char *address)
{
    setRemotePair(address);
}

Mantids30::Memory::Containers::B_Chunks *Socket_Stream_Dummy::getReceiver()
{
    return &receiver;
}

Mantids30::Memory::Containers::B_Chunks *Socket_Stream_Dummy::getSender()
{
    return &sender;
}
