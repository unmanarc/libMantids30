#include "socket_stream.h"
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

void Socket_Stream::writeEOF(bool)
{
    shutdownSocket(SHUT_RDWR);
}

bool Socket_Stream::streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus &wrsStat)
{
    char data[8192];
    Memory::Streams::WriteStatus cur;
    for (;;)
    {
        ssize_t r = partialRead(data,sizeof(data));
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

Memory::Streams::WriteStatus Socket_Stream::write(const void *buf, const size_t &count, Memory::Streams::WriteStatus &wrStat)
{
    Memory::Streams::WriteStatus cur;
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

std::pair<std::shared_ptr<Socket_Stream>, std::shared_ptr<Socket_Stream>> Socket_Stream::GetSocketPair()
{
    std::pair<std::shared_ptr<Socket_Stream>,std::shared_ptr<Socket_Stream>> p;

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
        p.first = std::make_shared<Socket_Stream>();
        p.second = std::make_shared<Socket_Stream>();

        p.first->setSocketFD(sockets[0]);
        p.second->setSocketFD(sockets[1]);
    }
#else
    // Emulate via TCP. (EXPERIMENTAL)

    std::shared_ptr<Sockets::Socket_TCP> llsock = std::make_shared<Socket_TCP>();
    std::shared_ptr<Sockets::Socket_TCP> rsock = std::make_shared<Socket_TCP>();
    std::shared_ptr<Sockets::Socket_TCP> lsock = nullptr;

    llsock->listenOn(0,"127.0.0.1");
    rsock->connectTo("127.0.0.1",lsock->getPort());
    lsock = std::dynamic_pointer_cast<Sockets::Socket_TCP>(llsock->acceptConnection());
    llsock->closeSocket();

    //delete llsock;

    p.first = lsock;
    p.second = rsock;
#endif
    return p;
}

bool Socket_Stream::writeFull(const void *data)
{
    return writeFull(data,strlen(((const char *)data)));
}

bool Socket_Stream::writeFull(const void *data, const uint64_t &datalen)
{
    // Init control variables:
    uint64_t remaining = datalen;           // data left to send
    const char* dataPtr = static_cast<const char*>(data); // data pointer.

    // Bucle para enviar datos en fragmentos hasta que se envíe todo
    while (remaining > 0)
    {
        // Determina el tamaño del fragmento (máximo 4096 bytes)
        uint64_t chunkSize = std::min(remaining, static_cast<uint64_t>(4096));

        // Envía el fragmento actual
        ssize_t sentBytes = partialWrite(dataPtr, static_cast<uint32_t>(chunkSize));

        // Manejo de errores
        if (sentBytes < 0)
        {
            // Error al enviar los datos
            shutdownSocket();
            return false;
        }

        if (sentBytes == 0)
        {
            // No se pudieron enviar bytes, posible error de conexión (buffer lleno?)
            shutdownSocket();
            return false;
        }

        // Actualiza el puntero y el conteo de datos pendientes
        dataPtr += sentBytes;
        remaining -= static_cast<uint64_t>(sentBytes);
    }

    // Todos los datos fueron enviados correctamente
    return true;
}


std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> Socket_Stream::acceptConnection()
{
    return nullptr;
}

bool Socket_Stream::postAcceptSubInitialization()
{
    return true;
}

bool Socket_Stream::postConnectSubInitialization()
{
    return true;
}


bool Socket_Stream::readFull(void *data, const uint64_t &expectedDataBytesCount, uint64_t * receivedDataBytesCount)
{
    if (receivedDataBytesCount != nullptr) {
        *receivedDataBytesCount = 0;
    }

    // Validate input
    if (data == nullptr) {
        return false;
    }

    if (expectedDataBytesCount == 0) {
        return true;
    }

    uint64_t curReceivedBytesCount = 0;
    const size_t MAX_READ_BUFFER_SIZE = 4096;

    while (curReceivedBytesCount < expectedDataBytesCount)
    {
        // Calcular el tamaño máximo a leer en esta iteración
        size_t bytesToRead = std::min<uint64_t>(MAX_READ_BUFFER_SIZE, expectedDataBytesCount - curReceivedBytesCount);

        ssize_t partialReceivedBytesCount = partialRead(
            static_cast<char*>(data) + curReceivedBytesCount,
            static_cast<uint32_t>(bytesToRead)
            );

        if (partialReceivedBytesCount < 0)
        {
            // Read Error.
            return false;
        }
        else if (partialReceivedBytesCount == 0)
        {
            // Connection Closed.
            break;
        }
        else
        {
            curReceivedBytesCount += static_cast<uint64_t>(partialReceivedBytesCount);
        }
    }

    // Notify the received bytes count.
    if (receivedDataBytesCount != nullptr)
    {
        *receivedDataBytesCount = curReceivedBytesCount;
    }

    // If we received less...
    if (curReceivedBytesCount < expectedDataBytesCount)
    {
        // Not interested in less.
        return false;
    }

    // We received complete:
    return true;
}
void Socket_Stream::deriveConnectionName()
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

void Socket_Stream::writeDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

void Socket_Stream::readDeSync()
{
    // Action when everything is desynced... (better to stop R/W from the socket)
    shutdownSocket();
}

bool Socket_Stream::isConnected()
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

bool Socket_Stream::listenOn(const uint16_t &, const char *, const int32_t &, const int32_t &)
{
    return false;
}

bool Socket_Stream::connectFrom(const char *, const char* , const uint16_t &, const uint32_t &)
{
	return false;
}
