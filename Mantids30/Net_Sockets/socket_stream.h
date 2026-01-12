#pragma once

#include "socket.h"
#include "socket_stream_reader.h"
#include "socket_stream_writer.h"
#include <Mantids30/Memory/streamable_object.h>
#include <memory>
#include <utility>

namespace Mantids30::Network::Sockets {

/*
This is the stream socket public definition for LLM:

namespace Mantids30::Network::Sockets {
class Socket_Stream : public Memory::Streams::StreamableObject, public Sockets::Socket, public Socket_Stream_Reader, public Socket_Stream_Writer
{
public:
    Socket_Stream() = default;
    virtual ~Socket_Stream() override = default;
    bool streamTo(Memory::Streams::StreamableObject *out) override;
    std::optional<size_t> write(const void * buf, const size_t &count) override;
    static std::pair<std::shared_ptr<Socket_Stream>, std::shared_ptr<Socket_Stream> > GetSocketPair();
    virtual bool isConnected() override;
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t & recvbuffer = 0, const int32_t &backlog = 10) override;
    virtual bool connectFrom(const char * bindAddress, const char * remoteHost, const uint16_t &port, const uint32_t &timeout = 30) override;
    virtual std::shared_ptr<Socket_Stream> acceptConnection();
    virtual bool writeFull(const void * buf);
    virtual bool writeFull(const void *data, const size_t &datalen) override;
    virtual bool readFull(void * data, const size_t &expectedDataBytesCount, size_t * receivedDataBytesCount = nullptr) override;
};

}

namespace Mantids30::Network::Sockets {
class Socket_Stream_Reader
{
public:
    Socket_Stream_Reader();
    virtual ~Socket_Stream_Reader();

    template<typename T> T readU(bool *readOK=nullptr);
    template<typename T> bool readBlockEx(void * data,T * datalen, bool mustReceiveFullDataLen = false);
    template<typename T> char *readBlockWAllocEx(T * datalen);
    char *readBlock32WAllocAndDelim(unsigned int * datalen, const char *delim, uint16_t delimBytes);
    template<typename T> std::string readStringEx(bool *readOK=nullptr , const T & maxLenght = std::numeric_limits<T>::max()-1);
};
}

class Socket_Stream_Writer
{
public:
    Socket_Stream_Writer() = default;
    virtual ~Socket_Stream_Writer() = default;
    template<typename T> bool writeU(const T & c);
    template<typename T> bool writeBlockEx(const void * data,const T & datalen);
    template<typename T> bool writeStringEx(const std::string & str,const size_t & maxSize = std::numeric_limits<T>::max()-1);
};
}
*/

class Socket_Stream : public Memory::Streams::StreamableObject, public Sockets::Socket, public Socket_Stream_Reader, public Socket_Stream_Writer
{
public:
    Socket_Stream() = default;
    virtual ~Socket_Stream() override = default;

    bool streamTo(Memory::Streams::StreamableObject *out) override;

    std::optional<size_t> write(const void *buf, const size_t &count) override;

    /**
     * @brief GetSocketPair Create a Pair of interconnected sockets
     * @return pair of interconnected Socket_Streams. (remember to delete them)
    */
    static std::pair<std::shared_ptr<Socket_Stream>, std::shared_ptr<Socket_Stream> > GetSocketPair();

    /**
     * @brief isConnected Get if the socket is connected. In case the socket is openned but the connection is down, the function will close the socket
     * @return true if connected, false otherwise.
     */
    virtual bool isConnected() override;

    // This methods are virtual and should be implemented in sub-classes.
    // TODO: virtual redefinition?
    virtual bool listenOn(const uint16_t &port, const char *listenOnAddr = "*", const int32_t &recvbuffer = 0, const int32_t &backlog = 10) override;
    virtual bool connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout = 30) override;
    virtual std::shared_ptr<Socket_Stream> acceptConnection();

    /**
     * Virtual function for protocol initialization after the connection starts...
     * useful for SSL server, it runs in blocking mode and should be called apart to avoid tcp accept while block
     * @return returns true if was properly initialized.
     */
    virtual bool postAcceptSubInitialization() { return true; }
    /**
     * Virtual function for protocol initialization after the connection starts (client-mode)...
     * useful for sub protocols initialization (eg. ssl), it runs in blocking mode.
     * @return returns true if was properly initialized.
     */
    virtual bool postConnectSubInitialization() { return true; }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Basic R/W options:
    /**
     * Write Null Terminated String on the socket
     * note: it writes the '\0' on the socket.
     * @param data null terminated string
     * @return true if the string was successfully sent
     */
    virtual bool writeFull(const void *buf);
    /**
     * Write a data block on the socket
     *
     * You can specify sizes bigger than 4k/8k, or megabytes (be careful with memory), and it will be fully sent in chunks.
     * @param data data block.
     * @param datalen data length in bytes
     * @return true if the data block was sucessfully sent.
     */
    virtual bool writeFull(const void *data, const size_t &datalen) override;
    /**
     * @brief readBlock Read a data block from the socket
     *                  Receive the data block in 4k chunks (or less) until it ends or fail.
     * @param data Memory address where the received data will be allocated
     * @param expectedDataBytesCount Expected data size (in bytes) to be read from the socket
     * @param receivedBytesCount if not null, this function returns the received byte count.
     * @return if it's disconnected (invalid sockfd) or it's not able to obtain at least 1 byte, then return false, otherwise true, you should check by yourself that expectedDataBytesCount == *receivedDataBytesCount
     */
    virtual bool readFull(void *data, const size_t &expectedDataBytesCount, size_t *receivedDataBytesCount = nullptr) override;

    void deriveConnectionName();


protected:
    void writeDeSync() override;
    void readDeSync() override;

};

typedef std::shared_ptr<Socket_Stream> Socket_Stream_SP;
} // namespace Mantids30::Network::Sockets
