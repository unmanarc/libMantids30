#ifndef BRIDGE_THREAD_BASE_H
#define BRIDGE_THREAD_BASE_H

#include "streamsocket.h"
#include <atomic>
#include <mutex>

namespace CX2 { namespace Network { namespace Streams {

class StreamsSocketsBridge_Thread
{
public:
    StreamsSocketsBridge_Thread();
    virtual ~StreamsSocketsBridge_Thread();

    void setStreamSockets(StreamSocket * src, StreamSocket * dst);
    /**
     * @brief processPipeFWD reads from SRC and write into DST making the proper transformations
     * @param src socket to read from.
     * @param dst socket to write into.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
    virtual int processPipeFWD();
    /**
     * @brief processPipeREV
     * @param src socket to read from.
     * @param dst socket to write into.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
    virtual int processPipeREV();

    virtual bool startPipeSync();
    /**
     * @brief setBlockSize Set Transfer Block Chunk Size
     * @param value Chunk size, default 8192
     */
    void setBlockSize(const uint32_t &value = 8192);
    /**
     * @brief writeBlock write a whole data into next socket
     * @param data data to be written
     * @param datalen data length to be written
     * @return true if success
     */
    bool writeBlockL(const void *data, const uint32_t &datalen, bool fwd = true);

protected:
    StreamSocket * src;
    char * block_fwd;
    std::atomic<unsigned int> blockSize;
    int partialReadL(void *data, const uint32_t &datalen, bool fwd = true);

private:
    /**
     * @brief simpleProcessPipe simple pipe processor (data in, data out as is)
     * @param src socket to read from.
     * @param dst socket to read from.
     * @return -1 if src terminated the connection, -2 if dst terminated the connection, otherwise, bytes processed.
     */
    int simpleProcessPipe(bool fwd);


    StreamSocket * dst;
    char * block_rev;

    std::mutex mt_fwd, mt_rev;
};

}}}

#endif // BRIDGE_THREAD_BASE_H
