#include "streams_bridge_thread.h"
#include <cstdint>
#include <memory>

#ifndef WIN32
#include <netinet/in.h>
#endif

using namespace Mantids30::Network::Sockets;
using namespace NetStreams;



Bridge_Thread::~Bridge_Thread()
{
    delete[] block_fwd;
    delete[] m_block_rev;
}

void Bridge_Thread::setSocketEndpoints(const std::shared_ptr<Socket_Stream> &src, const std::shared_ptr<Socket_Stream> &dst, bool chunked)
{
    this->src = src;
    this->m_dstSocket = dst;
    this->m_chunked = chunked;
}

bool Bridge_Thread::sendPing()
{
    std::lock_guard<std::mutex> lock(mt_fwd);
    bool r = m_dstSocket->writeU<uint16_t>((uint16_t) 0);
    return r;
}

bool Bridge_Thread::startPipeSync()
{
    return true;
}

void Bridge_Thread::setBlockSize(uint16_t value)
{
    delete[] block_fwd;
    delete[] m_block_rev;

    blockSize = value;

    block_fwd = new char[value];
    m_block_rev = new char[value];
}

void Bridge_Thread::terminate()
{
    m_terminated = true;
}

int Bridge_Thread::processPipe(Side fwd)
{
    char *curBlock = fwd == Side::FORWARD ? block_fwd : m_block_rev;

    int bytesReceived;

    if (!m_chunked)
    {
        // Stream mode: read and write from the both peers

        // TODO: if writer is done...
        if ((bytesReceived = (fwd == Side::FORWARD ? src : m_dstSocket)->partialRead(curBlock, blockSize)) > 0)
        {
            {
                std::lock_guard<std::mutex> lock(fwd == Side::FORWARD ? mt_fwd : mt_rev);

                if (!(fwd == Side::FORWARD ? m_dstSocket : src)->writeFull(curBlock, bytesReceived))
                {
                    return -2;
                }
            }

            // Return for Update Counters:
            return bytesReceived;
        }
    }
    else
    {
        if (fwd == Side::FORWARD)
        {
            // 0->1 (encapsulate)

            // Read the raw stream:
            bytesReceived = src->partialRead(curBlock, blockSize);

            if (bytesReceived > 0)
            {
                std::lock_guard<std::mutex> lock(mt_fwd);

                // Write the size to be written (chunk)
                if (!m_dstSocket->writeU<uint16_t>((uint16_t) bytesReceived))
                {
                    return -2;
                }

                // Write the packet itself.
                if (!m_dstSocket->writeFull(curBlock, bytesReceived))
                {
                    return -2;
                }

                return bytesReceived;
            }
        }
        else
        {
            // 1->0 (decapsulate)
            bool readOK;
            bytesReceived = m_dstSocket->readU<uint16_t>(&readOK);
            if (!readOK)
            {
                return -1;
            }

            // It's a ping! (do nothing, but validate that the connection is not terminated yet)
            if (bytesReceived == 0)
            {
                if (m_terminated)
                {
                    return -2;
                }

                return -3;
            }

            // Attempt to read from the dst
            if (!m_dstSocket->readFull(curBlock, (uint16_t) bytesReceived))
            {
                return -1;
            }

            // Attempt to write to the src
            if (!src->writeFull(curBlock, (uint16_t) bytesReceived))
            {
                return -2;
            }

            return bytesReceived;
        }
    }

    return -1;
}
