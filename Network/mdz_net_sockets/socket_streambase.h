#ifndef SOCKET_STREAMBASE_H
#define SOCKET_STREAMBASE_H

#include "socket.h"
#include "socket_streambasereader.h"
#include "socket_streambasewriter.h"
#include <utility>
#include <mdz_mem_vars/streamableobject.h>

namespace Mantids { namespace Network { namespace Sockets {

class Socket_StreamBase : public Memory::Streams::StreamableObject, public Sockets::Socket, public Socket_StreamBaseReader, public Socket_StreamBaseWriter
{
public:
    Socket_StreamBase();
    virtual ~Socket_StreamBase()override;

    virtual void writeEOF(bool) override;

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::StreamableObject::Status & wrsStat) override;

    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Memory::Streams::StreamableObject::Status & wrStatUpd) override;

    /**
     * @brief GetSocketPair Create a Pair of interconnected sockets
     * @return pair of interconnected Socket_StreamBases. (remember to delete them)
    */
    static std::pair<Socket_StreamBase *, Socket_StreamBase *> GetSocketPair();

    virtual bool isConnected() override;
    // This methods are virtual and should be implemented in sub-classes.
    // TODO: virtual redefinition?
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t & recvbuffer = 0, const int32_t &backlog = 10) override;
    virtual bool connectFrom(const char * bindAddress, const char * remoteHost, const uint16_t &port, const uint32_t &timeout = 30) override;
    virtual Socket_StreamBase * acceptConnection();

    /**
     * Virtual function for protocol initialization after the connection starts...
     * useful for SSL server, it runs in blocking mode and should be called apart to avoid tcp accept while block
     * @return returns true if was properly initialized.
     */
    virtual bool postAcceptSubInitialization();
    /**
     * Virtual function for protocol initialization after the connection starts (client-mode)...
     * useful for sub protocols initialization (eg. ssl), it runs in blocking mode.
     * @return returns true if was properly initialized.
     */
    virtual bool postConnectSubInitialization();


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Basic R/W options:
    /**
     * Write Null Terminated String on the socket
     * note: it writes the '\0' on the socket.
     * @param data null terminated string
     * @return true if the string was successfully sent
     */
    virtual bool writeFull(const void * buf);
    /**
     * Write a data block on the socket
     *
     * You can specify sizes bigger than 4k/8k, or megabytes (be careful with memory), and it will be fully sent in chunks.
     * @param data data block.
     * @param datalen data length in bytes
     * @return true if the data block was sucessfully sent.
     */
    virtual bool writeFull(const void * data, const uint64_t & datalen) override;
    /**
     * @brief readBlock Read a data block from the socket
     *                  Receive the data block in 4k chunks (or less) until it ends or fail.
     * @param data Memory address where the received data will be allocated
     * @param expectedDataBytesCount Expected data size (in bytes) to be read from the socket
     * @param receivedBytesCount if not null, this function returns the received byte count.
     * @return if it's disconnected (invalid sockfd) or it's not able to obtain at least 1 byte, then return false, otherwise true, you should check by yourself that expectedDataBytesCount == *receivedDataBytesCount
     */
    virtual bool readFull(void * data, const uint64_t &expectedDataBytesCount, uint64_t * receivedDataBytesCount = nullptr) override;


protected:
    void writeDeSync() override;
    void readDeSync() override;
};

typedef std::shared_ptr<Socket_StreamBase> Socket_StreamBase_SP;
}}}


#endif /* SOCKET_STREAMBASE_H */
