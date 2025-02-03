#include "socket_stream_base.h"
#include <memory>

#ifndef _WIN32
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include "socket_tcp.h"

#endif
#include <string.h>
#include <unistd.h>

#include <Mantids30/Helpers/random.h>

using namespace Mantids30;
using namespace Mantids30::Network::Sockets;

void Socket_Stream_Base::writeEOF(bool)
{
    shutdownSocket(SHUT_RDWR);
}

bool Socket_Stream_Base::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> out, Memory::Streams::StreamableObject::Status &wrsStat)
{
    char data[8192];
    Memory::Streams::StreamableObject::Status cur;
    for (;;)
    {
        int r = partialRead(data,sizeof(data));
        switch (r)
        {
        case -1: // ERR.
            out->writeEOF(false);
            return false;
        case 0: // EOF.
            out->writeEOF(true);
            return true;
        default:
            if (!(cur=out->writeFullStream(data,r,wrsStat)).succeed || cur.finish)
            {
                if (!cur.succeed)
                {
                    out->writeEOF(false);
                    return false;
                }
                else
                {
                    out->writeEOF(true);
                    return true;
                }
            }
        break;
        }
    }
}

Memory::Streams::StreamableObject::Status Socket_Stream_Base::write(const void *buf, const size_t &count, Memory::Streams::StreamableObject::Status &wrStat)
{
    Memory::Streams::StreamableObject::Status cur;
    // TODO: report the right amount of data copied...
    bool r = writeFull(buf,count);
    if (!r)
        wrStat.succeed=cur.succeed=setFailedWriteState();
    else
    {
        cur.bytesWritten+=count;
        wrStat.bytesWritten+=count;
    }
    return cur;
}

std::pair<std::shared_ptr<Socket_Stream_Base>, std::shared_ptr<Socket_Stream_Base>> Socket_Stream_Base::GetSocketPair()
{
    std::pair<std::shared_ptr<Socket_Stream_Base>,std::shared_ptr<Socket_Stream_Base>> p;

    p.first = nullptr;
    p.second = nullptr;

#ifndef _WIN32
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
        // ERROR:...
    }
    else
    {
        p.first = std::make_shared<Socket_Stream_Base>();
        p.second = std::make_shared<Socket_Stream_Base>();

        p.first->setSocketFD(sockets[0]);
        p.second->setSocketFD(sockets[1]);
    }
#else
    // Emulate via TCP. (EXPERIMENTAL)

    std::shared_ptr<Sockets::Socket_TCP> llsock = std::make_shared<Socket_TCP>();
    std::shared_ptr<Sockets::Socket_TCP> rsock = std::make_shared<Socket_TCP>();
    std::shared_ptr<Sockets::Socket_TCP> lsock = nullptr;

    llsock->listenOn(0,"127.0.0.1",true);
    rsock->connectTo("127.0.0.1",lsock->getPort());
    lsock = std::dynamic_pointer_cast<Sockets::Socket_TCP>(llsock->acceptConnection());
    llsock->closeSocket();

    //delete llsock;

    p.first = lsock;
    p.second = rsock;
#endif
    return p;
}

bool Socket_Stream_Base::writeFull(const void *data)
{
    return writeFull(data,strlen(((const char *)data)));
}

bool Socket_Stream_Base::writeFull(const void *data, const uint64_t &datalen)
{
    uint64_t sent_bytes = 0;
    uint64_t left_to_send = datalen;

    // Send the raw data.
    // datalen-left_to_send is the _size_ of the data already sent.
    while (left_to_send && (sent_bytes = partialWrite((char *) data + (datalen - left_to_send), left_to_send>4096?4096:left_to_send)) <= left_to_send)
    {
        if (sent_bytes == -1)
        {
            // Error sending data. (returns false.)
            shutdownSocket();
            return false;
        }
        // Substract the data that was already sent from the count.
        else
            left_to_send -= sent_bytes;
    }

    // Failed to achieve sending the contect on 5 attempts
    if (left_to_send != 0)
    {
        // left_to_send must always return 0 bytes. otherwise here we have an error (return false)
        shutdownSocket();
        return false;
    }
    return true;
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream_Base> Socket_Stream_Base::acceptConnection()
{
    return nullptr;
}

bool Socket_Stream_Base::postAcceptSubInitialization()
{
    return true;
}

bool Socket_Stream_Base::postConnectSubInitialization()
{
    return true;
}


bool Socket_Stream_Base::readFull(void *data, const uint64_t &expectedDataBytesCount, uint64_t * receivedDataBytesCount)
{
    if (receivedDataBytesCount) *receivedDataBytesCount = 0;

    uint64_t curReceivedBytesCount = 0;
    int partialReceivedBytesCount = 0;

    if (expectedDataBytesCount==0)
        return true;

    // Try to receive the maximum amount of data left.
    while ( (expectedDataBytesCount - curReceivedBytesCount)>0 // there are bytes to read.
            && (partialReceivedBytesCount = partialRead(((char *) data) + curReceivedBytesCount, expectedDataBytesCount - curReceivedBytesCount)) >0 // receive bytes. if error, will return with -1 or zero (connection terminated).
            )
    {
        // Count the data received.
        curReceivedBytesCount += partialReceivedBytesCount;
    }

    if (curReceivedBytesCount<expectedDataBytesCount)
    {
        if (curReceivedBytesCount==0)
            return false;
        if (receivedDataBytesCount) *receivedDataBytesCount = curReceivedBytesCount;
        return true;
    }

    if (receivedDataBytesCount) *receivedDataBytesCount = expectedDataBytesCount;

    // Otherwise... return true.
    return true;
}

void Socket_Stream_Base::deriveConnectionName()
{
    std::string rpcClientKey;
    std::string remotePair = getRemotePairStr();

    // TLS peer name:
    std::string peerName = getPeerName();

    if (peerName.empty())
    {
        // TCP connection or TLS without cert/CN? use the remote IP/port...
        peerName = remotePair + ":" + std::to_string(getRemotePort());
    }

    // Multiple client connection using this peer name? give a random suffix...
    rpcClientKey = peerName + "?" + Helpers::Random::createRandomHexString(8);

    setConnectionName(rpcClientKey);
}

void Socket_Stream_Base::writeDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

void Socket_Stream_Base::readDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

bool Socket_Stream_Base::isConnected()
{
    if (!isActive())
        return false;

    struct sockaddr peer;
    socklen_t peer_len;
    peer_len = sizeof(peer);
    if (getpeername(m_sockFD, &peer, &peer_len) == -1)
    {
        closeSocket();
        return false;
    }
    return true;
}

bool Socket_Stream_Base::listenOn(const uint16_t &, const char *, const int32_t &, const int32_t &)
{
    return false;
}

bool Socket_Stream_Base::connectFrom(const char *, const char* , const uint16_t &, const uint32_t &)
{
	return false;
}
