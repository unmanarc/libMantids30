#include "socket_streambase.h"

#ifndef _WIN32
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include "socket_tcp.h"

#endif
#include <string.h>
#include <unistd.h>

using namespace Mantids;
using namespace Mantids::Network::Sockets;

Socket_StreamBase::Socket_StreamBase()
{
    //printf("Creating streamsocket %p\n", this); fflush(stdout);
}

Socket_StreamBase::~Socket_StreamBase()
{
  //  printf("Removing streamsocket %p\n", this); fflush(stdout);
}

void Socket_StreamBase::writeEOF(bool)
{
    shutdownSocket(SHUT_RDWR);
}

bool Socket_StreamBase::streamTo(Memory::Streams::StreamableObject *out, Memory::Streams::StreamableObject::Status &wrsStat)
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

Memory::Streams::StreamableObject::Status Socket_StreamBase::write(const void *buf, const size_t &count, Memory::Streams::StreamableObject::Status &wrStat)
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

std::pair<Socket_StreamBase *,Socket_StreamBase *> Socket_StreamBase::GetSocketPair()
{
    std::pair<Socket_StreamBase *,Socket_StreamBase *> p;

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
        p.first = new Socket_StreamBase();
        p.second = new Socket_StreamBase();

        p.first->setSocketFD(sockets[0]);
        p.second->setSocketFD(sockets[1]);
    }
#else
    // Emulate via TCP. (EXPERIMENTAL)

    Sockets::Socket_TCP * llsock = new Sockets::Socket_TCP, * lsock = nullptr, * rsock = new Sockets::Socket_TCP;
    llsock->listenOn(0,"127.0.0.1",true);
    rsock->connectTo("127.0.0.1",lsock->getPort());
    lsock = (Sockets::Socket_TCP *)llsock->acceptConnection();
    llsock->closeSocket();
    delete llsock;

    p.first = lsock;
    p.second = rsock;
#endif
    return p;
}

bool Socket_StreamBase::writeFull(const void *data)
{
    return writeFull(data,strlen(((const char *)data)));
}

bool Socket_StreamBase::writeFull(const void *data, const uint64_t &datalen)
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

Socket_StreamBase * Socket_StreamBase::acceptConnection()
{
    return nullptr;
}

bool Socket_StreamBase::postAcceptSubInitialization()
{
    return true;
}

bool Socket_StreamBase::postConnectSubInitialization()
{
    return true;
}


bool Socket_StreamBase::readFull(void *data, const uint64_t &expectedDataBytesCount, uint64_t * receivedDataBytesCount)
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

void Socket_StreamBase::writeDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

void Socket_StreamBase::readDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

bool Socket_StreamBase::isConnected()
{
    if (!isActive())
        return false;

    struct sockaddr peer;
    socklen_t peer_len;
    peer_len = sizeof(peer);
    if (getpeername(sockfd, &peer, &peer_len) == -1)
    {
        closeSocket();
        return false;
    }
    return true;
}

bool Socket_StreamBase::listenOn(const uint16_t &, const char *, const int32_t &, const int32_t &)
{
    return false;
}

bool Socket_StreamBase::connectFrom(const char *, const char* , const uint16_t &, const uint32_t &)
{
	return false;
}
