#include "streams_bridge.h"

using namespace Mantids::Network::Sockets;
using namespace NetStreams;

Bridge::Bridge()
{
    pingEveryMS = 5000;
    bridgeThreadPrc = nullptr;
    sentBytes = 0;
    recvBytes = 0;

    peers[0] = nullptr;
    peers[1] = nullptr;

    finishingPeer = -1;
    autoDeleteSocketsOnExit = false;
    autoDeleteCustomPipeOnClose = false;
    transmitionMode = TRANSMITION_MODE_STREAM;

    setAutoDeleteStreamPipeOnThreadExit(true);
    setToShutdownRemotePeer(true);
    setToCloseRemotePeer(true);
}

Bridge::~Bridge()
{
    if (autoDeleteSocketsOnExit)
    {
        if (peers[0])
            delete peers[0];
        if (peers[1])
            delete peers[1];
    }
}

void Bridge::pipeThread(Bridge *stp)
{
    stp->process();
    if (stp->isAutoDeleteStreamPipeOnThreadExit())
    {
        delete stp;
    }
}

uint32_t Bridge::getPingEveryMS() const
{
    return pingEveryMS;
}

void Bridge::setPingEveryMS(uint32_t newPingEveryMS)
{
    pingEveryMS = newPingEveryMS;
}

void Bridge::remotePeerThread(Bridge *stp)
{
    stp->processPeer(1);
}

void Bridge::pingThread(Bridge *stp)
{
    stp->sendPing();
}

bool Bridge::start(bool _autoDeleteStreamPipeOnExit, bool detach)
{
    if (!peers[0] || !peers[1])
        return false;

    autoDeleteStreamPipeOnExit = _autoDeleteStreamPipeOnExit;

    pipeThreadP = std::thread(pipeThread, this);

    if (autoDeleteStreamPipeOnExit || detach)
        pipeThreadP.detach();

    return true;
}

void Bridge::sendPing()
{
    std::unique_lock<std::mutex> lock(mutex_endPingLoop);
    using Ms = std::chrono::milliseconds;

    while(!pingFinished)
    {
        if (cond_endPing.wait_for(lock,Ms(pingEveryMS)) == std::cv_status::timeout)
        {
            // Send some ping...
            bridgeThreadPrc->sendPing();
        }
        else
        {
            // notified to terminate pings...
        }
    }
}

int Bridge::wait()
{
    pipeThreadP.join();
    return finishingPeer;
}

int Bridge::process()
{
    if (!peers[0] || !peers[1])
        return -1;

    if (!bridgeThreadPrc)
    {
        bridgeThreadPrc = new Bridge_Thread();
        autoDeleteCustomPipeOnClose = true;
    }

    bridgeThreadPrc->setSocketEndpoints(peers[0],peers[1], transmitionMode == TRANSMITION_MODE_CHUNKSANDPING);

    if (bridgeThreadPrc->startPipeSync())
    {
        pingFinished = false;
        std::thread pingerThread;
        if (transmitionMode == TRANSMITION_MODE_CHUNKSANDPING)
        {
             pingerThread = std::thread(pingThread, this);
        }

        std::thread remotePeerThreadP = std::thread(remotePeerThread, this);
        processPeer(0);
        remotePeerThreadP.join();

        // Wait until the ping loop is in "rest mode"
        if (transmitionMode == TRANSMITION_MODE_CHUNKSANDPING)
        {
            std::unique_lock<std::mutex> lock(mutex_endPingLoop);
            // and notify that there is no more pings
            pingFinished = true;
            cond_endPing.notify_one();
            pingerThread.join();
        }

    }

    // All connections terminated.
    if (closeRemotePeerOnFinish)
    {
        // close them also.
        peers[1]->closeSocket();
        peers[0]->closeSocket();
    }

    if ( autoDeleteCustomPipeOnClose )
    {
        delete bridgeThreadPrc;
        bridgeThreadPrc = nullptr;
    }

    return finishingPeer;
}

bool Bridge::processPeer(unsigned char cur)
{
    if (cur>1)
        return false;

    unsigned char next = cur==0?1:0;
    std::atomic<uint64_t> * bytesCounter = cur==0?&sentBytes:&recvBytes;

    int dataRecv=0;
    while ( dataRecv >= 0 )
    {
        dataRecv = bridgeThreadPrc->processPipe(cur==0); // ?bridgeThreadPrc->processPipeFWD():bridgeThreadPrc->processPipeREV();

        if (dataRecv>=0)
            *bytesCounter+=dataRecv;

        else if (dataRecv==-1 && shutdownRemotePeerOnFinish)
        {
            peers[next]->shutdownSocket();
            finishingPeer = cur;
        }
    }

    return true;
}

bool Bridge::setPeer(unsigned char i, Socket_StreamBase *s)
{
    if (i>1)
        return false;
    peers[i] = s;
    return true;
}

Socket_StreamBase *Bridge::getPeer(unsigned char i)
{
    if (i>1)
        return nullptr;
    return peers[i];
}

void Bridge::setAutoDeleteStreamPipeOnThreadExit(bool value)
{
    autoDeleteStreamPipeOnExit = value;
}

void Bridge::setToShutdownRemotePeer(bool value)
{
    shutdownRemotePeerOnFinish = value;
}

void Bridge::setToCloseRemotePeer(bool value)
{
    closeRemotePeerOnFinish = value;
}

uint64_t Bridge::getSentBytes() const
{
    return sentBytes;
}

uint64_t Bridge::getRecvBytes() const
{
    return recvBytes;
}

bool Bridge::isAutoDeleteStreamPipeOnThreadExit() const
{
    return autoDeleteStreamPipeOnExit;
}

bool Bridge::isAutoDeleteSocketsOnExit() const
{
    return autoDeleteSocketsOnExit;
}

void Bridge::setAutoDeleteSocketsOnExit(bool value)
{
    autoDeleteSocketsOnExit = value;
}

// TODO: deleteOnExit

void Bridge::setCustomPipeProcessor(Bridge_Thread *value, bool deleteOnExit)
{
    bridgeThreadPrc = value;
}

Bridge::TransmitionMode Bridge::getTransmitionMode() const
{
    return transmitionMode;
}

void Bridge::setTransmitionMode(TransmitionMode newTransmitionMode)
{
    transmitionMode = newTransmitionMode;
}

