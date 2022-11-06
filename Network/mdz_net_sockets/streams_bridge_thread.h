#ifndef BRIDGE_THREAD_BASE_H
#define BRIDGE_THREAD_BASE_H

#include "socket_streambase.h"
#include <atomic>
#include <mutex>

namespace Mantids { namespace Network { namespace Sockets { namespace NetStreams {

class Bridge_Thread
{
public:
    Bridge_Thread();
    virtual ~Bridge_Thread();

    void setSocketEndpoints(Sockets::Socket_StreamBase * src, Sockets::Socket_StreamBase * dst, bool chunked);
    /**
     * @brief processPipeFWD reads from SRC and write into DST making the proper transformations
     * @param src socket to read from.
     * @param dst socket to write into.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
  //  virtual int processPipeFWD();
    /**
     * @brief processPipeREV
     * @param src socket to read from.
     * @param dst socket to write into.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
//    virtual int processPipeREV();

    bool sendPing();

    /**
     * @brief simpleProcessPipe simple pipe processor (data in, data out as is)
     * @param src socket to read from.
     * @param dst socket to read from.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
    int processPipe(bool fwd);


    virtual bool startPipeSync();
    /**
     * @brief setBlockSize Set Transfer Block Chunk Size
     * @param value Chunk size, default 8192
     */
    void setBlockSize(uint16_t value = 8192);
    /**
     * @brief writeBlock write a whole data into next socket
     * @param data data to be written
     * @param datalen data length to be written
     * @return true if success
     */
    bool writeBlockL(const void *data, const uint32_t &datalen, bool fwd = true);

protected:
    Sockets::Socket_StreamBase * src;
    char * block_fwd;
    uint16_t blockSize;
   // int partialReadL(void *data, const uint32_t &datalen, bool fwd = true);

private:


    bool chunked;

    Sockets::Socket_StreamBase * dst;
    char * block_rev;

    std::mutex mt_fwd, mt_rev;
};

}}}}

#endif // BRIDGE_THREAD_BASE_H
