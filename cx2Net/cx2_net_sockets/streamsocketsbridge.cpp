#include "streamsocketsbridge.h"

using namespace CX2::Network::Streams;



StreamSocketsBridge::StreamSocketsBridge()
{
    bridgeThreadPrc = nullptr;
    sentBytes = 0;
    recvBytes = 0;

    socket_peers[0] = nullptr;
    socket_peers[1] = nullptr;

    finishingPeer = -1;
    autoDeleteSocketsOnExit = false;
    autoDeleteCustomPipeOnClose = false;

    setAutoDeleteStreamPipeOnThreadExit(true);
    setToShutdownRemotePeer(true);
    setToCloseRemotePeer(true);
}

StreamSocketsBridge::~StreamSocketsBridge()
{
    if (autoDeleteSocketsOnExit)
    {
        if (socket_peers[0]) delete socket_peers[0];
        if (socket_peers[1]) delete socket_peers[1];
    }
}

bool StreamSocketsBridge::start(bool _autoDeleteStreamPipeOnExit, bool detach)
{
    if (!socket_peers[0] || !socket_peers[1]) return false;

    autoDeleteStreamPipeOnExit = _autoDeleteStreamPipeOnExit;

    pipeThreadP = std::thread(pipeThread, this);

    if (autoDeleteStreamPipeOnExit || detach)
        pipeThreadP.detach();

    return true;
}

int StreamSocketsBridge::wait()
{
    pipeThreadP.join();
    return finishingPeer;
}


int StreamSocketsBridge::process()
{
    if (!socket_peers[0] || !socket_peers[1]) return -1;

    if (!bridgeThreadPrc)
    {
        bridgeThreadPrc = new StreamsSocketsBridge_Thread();
        autoDeleteCustomPipeOnClose = true;
    }

    bridgeThreadPrc->setStreamSockets(socket_peers[0],socket_peers[1]);

    if (bridgeThreadPrc->startPipeSync())
    {
        std::thread remotePeerThreadP = std::thread(remotePeerThread, this);
        processPeer(0);
        remotePeerThreadP.join();
    }

    // All connections terminated.
    if (closeRemotePeerOnFinish)
    {
        // close them also.
        socket_peers[1]->closeSocket();
        socket_peers[0]->closeSocket();
    }

    if ( autoDeleteCustomPipeOnClose )
    {
        delete bridgeThreadPrc;
        bridgeThreadPrc = nullptr;
    }

    return finishingPeer;
}

bool StreamSocketsBridge::processPeer(unsigned char cur)
{
    if (cur>1) return false;

    unsigned char next = cur==0?1:0;
    std::atomic<uint64_t> * bytesCounter = cur==0?&sentBytes:&recvBytes;

    int dataRecv=0;
    while ( dataRecv >= 0 )
    {
        dataRecv = cur==0?bridgeThreadPrc->processPipeFWD():bridgeThreadPrc->processPipeREV();

        if (dataRecv>=0) *bytesCounter+=dataRecv;
        else if (dataRecv==-1 && shutdownRemotePeerOnFinish)
        {
            socket_peers[next]->shutdownSocket();
            finishingPeer = cur;
        }
    }

    return true;
}

bool StreamSocketsBridge::setPeer(unsigned char i, StreamSocket *s)
{
    if (i>1) return false;
    socket_peers[i] = s;
    return true;
}

StreamSocket *StreamSocketsBridge::getPeer(unsigned char i)
{
    if (i>1) return nullptr;
    return socket_peers[i];
}

void StreamSocketsBridge::setAutoDeleteStreamPipeOnThreadExit(bool value)
{
    autoDeleteStreamPipeOnExit = value;
}

void StreamSocketsBridge::setToShutdownRemotePeer(bool value)
{
    shutdownRemotePeerOnFinish = value;
}

void StreamSocketsBridge::setToCloseRemotePeer(bool value)
{
    closeRemotePeerOnFinish = value;
}

uint64_t StreamSocketsBridge::getSentBytes() const
{
    return sentBytes;
}

uint64_t StreamSocketsBridge::getRecvBytes() const
{
    return recvBytes;
}

bool StreamSocketsBridge::isAutoDeleteStreamPipeOnThreadExit() const
{
    return autoDeleteStreamPipeOnExit;
}

bool StreamSocketsBridge::isAutoDeleteSocketsOnExit() const
{
    return autoDeleteSocketsOnExit;
}

void StreamSocketsBridge::setAutoDeleteSocketsOnExit(bool value)
{
    autoDeleteSocketsOnExit = value;
}

// TODO: deleteOnExit

void StreamSocketsBridge::setCustomPipeProcessor(StreamsSocketsBridge_Thread *value, bool deleteOnExit)
{
    bridgeThreadPrc = value;
}

void StreamSocketsBridge::remotePeerThread(StreamSocketsBridge *stp)
{
    stp->processPeer(1);
}

void StreamSocketsBridge::pipeThread(StreamSocketsBridge *stp)
{
    stp->process();
    if (stp->isAutoDeleteStreamPipeOnThreadExit())
    {
        delete stp;
    }
}
