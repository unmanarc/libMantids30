#include "streamsocketsbridge_thread.h"

using namespace Mantids::Network::Streams;

StreamsSocketsBridge_Thread::StreamsSocketsBridge_Thread()
{
    block_fwd = nullptr;
    block_rev = nullptr;
    setBlockSize(8192);
}

StreamsSocketsBridge_Thread::~StreamsSocketsBridge_Thread()
{
    delete [] block_fwd;
    delete [] block_rev;
}

void StreamsSocketsBridge_Thread::setStreamSockets(StreamSocket *src, StreamSocket *dst)
{
    this->src = src;
    this->dst = dst;
}

int StreamsSocketsBridge_Thread::processPipeFWD()
{
    return simpleProcessPipe(true);
}

int StreamsSocketsBridge_Thread::processPipeREV()
{
    return simpleProcessPipe(false);
}

bool StreamsSocketsBridge_Thread::startPipeSync()
{
    return true;
}

void StreamsSocketsBridge_Thread::setBlockSize(const uint32_t & value)
{
    if (block_fwd) delete [] block_fwd;
    if (block_rev) delete [] block_rev;
    blockSize = value;
    block_fwd = new char[value];
    block_rev = new char[value];
}

bool StreamsSocketsBridge_Thread::writeBlockL(const void *data, const uint32_t & datalen, bool fwd)
{
    std::lock_guard<std::mutex> lock(fwd?mt_fwd:mt_rev);
    StreamSocket *dstX=fwd?dst:src;
    return dstX->writeFull(data,datalen);
}

int StreamsSocketsBridge_Thread::simpleProcessPipe(bool fwd)
{
    char * curBlock = fwd?block_fwd:block_rev;

    int bytesReceived;
    if ((bytesReceived=partialReadL(curBlock,blockSize,fwd))>0)
    {
        if (!writeBlockL(curBlock,bytesReceived,fwd)) return -2;
        // Update Counters:
        return bytesReceived;
    }

    return -1;
}

int StreamsSocketsBridge_Thread::partialReadL(void *data, const uint32_t &datalen, bool fwd)
{
    return (fwd?src:dst)->partialRead(data,datalen);
}
