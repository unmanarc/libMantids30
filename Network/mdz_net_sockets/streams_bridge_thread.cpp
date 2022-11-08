#include "streams_bridge_thread.h"
#include <cstdint>

#ifndef WIN32
#include <netinet/in.h>
#endif

using namespace Mantids::Network::Sockets;
using namespace NetStreams;

Bridge_Thread::Bridge_Thread()
{
    block_fwd = nullptr;
    block_bwd = nullptr;
    chunked = false;
    terminated = false;
    setBlockSize(8192);
}

Bridge_Thread::~Bridge_Thread()
{
    delete [] block_fwd;
    delete [] block_bwd;
}

void Bridge_Thread::setSocketEndpoints(Socket_StreamBase *src, Socket_StreamBase *dst, bool chunked)
{
    this->src = src;
    this->dst = dst;
    this->chunked = chunked;
}

bool Bridge_Thread::sendPing()
{
    std::lock_guard<std::mutex> lock(mt_fwd);
    bool r = dst->writeU<uint16_t>((uint16_t)0);
    return r;
}

bool Bridge_Thread::startPipeSync()
{
    return true;
}

void Bridge_Thread::setBlockSize(uint16_t value)
{
    if (block_fwd)
        delete [] block_fwd;
    if (block_bwd)
        delete [] block_bwd;

    blockSize = value;

    block_fwd = new char[value];
    block_bwd = new char[value];
}

void Bridge_Thread::terminate()
{
    terminated = true;
}

int Bridge_Thread::processPipe(Side fwd)
{
    char * curBlock = fwd==SIDE_FORWARD?block_fwd:block_bwd;

    int bytesReceived;

    if ( !chunked )
    {
        // Stream mode: read and write from the both peers

        // TODO: if writer is done...
        if ((bytesReceived=
             (fwd==SIDE_FORWARD?src:dst)->partialRead(curBlock,blockSize)
             )>0)
        {
            {
                std::lock_guard<std::mutex> lock(fwd==SIDE_FORWARD?mt_fwd:mt_rev);

                if (!(fwd==SIDE_FORWARD?dst:src)->writeFull(curBlock,bytesReceived))
                    return -2;
            }

            // Return for Update Counters:
            return bytesReceived;
        }
    }
    else
    {
        if (fwd == SIDE_FORWARD)
        {
            // 0->1 (encapsulate)

            // Read the raw stream:
            bytesReceived = src->partialRead(curBlock,blockSize);

            if ( bytesReceived > 0 )
            {
                std::lock_guard<std::mutex> lock(mt_fwd);

                // Write the size to be written (chunk)
                if (!dst->writeU<uint16_t>((uint16_t)bytesReceived))
                    return -2;

                // Write the packet itself.
                if (!dst->writeFull(curBlock,bytesReceived))
                    return -2;

                return bytesReceived;
            }

        }
        else
        {
            // 1->0 (decapsulate)
            bool readOK;
            bytesReceived = dst->readU<uint16_t>(&readOK);
            if (!readOK)
                return -1;

            // It's a ping! (do nothing, but validate that the connection is not terminated yet)
            if (bytesReceived == 0)
            {
                if (terminated)
                    return -2;

                return -3;
            }

            // Attempt to read from the dst
            if (!dst->readFull(curBlock,(uint16_t)bytesReceived))
                return -1;

            // Attempt to write to the src
            if (!src->writeFull(curBlock,(uint16_t)bytesReceived))
                return -2;

            return bytesReceived;
        }
    }

    return -1;
}
