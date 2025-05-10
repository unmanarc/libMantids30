#include "streams_bridge.h"
#include <mutex>

using namespace Mantids30::Network::Sockets;
using namespace NetStreams;

Bridge::Bridge()
{
    setAutoDeleteStreamPipeOnThreadExit(true);
    setToShutdownRemotePeer(true);
    setToCloseRemotePeer(true);
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
    return m_pingEveryMS;
}

void Bridge::setPingEveryMS(uint32_t newPingEveryMS)
{
    m_pingEveryMS = newPingEveryMS;
}

void Bridge::remotePeerThread(Bridge *stp)
{
    stp->processPeer(SIDE_BACKWARD);
}

void Bridge::pingThread(Bridge *stp)
{
    stp->sendPing();
}

bool Bridge::start(bool _autoDeleteStreamPipeOnExit, bool detach)
{
    if (!m_peers[0] || !m_peers[1])
        return false;

    m_autoDeleteStreamPipeOnExit = _autoDeleteStreamPipeOnExit;

    m_pipeThreadP = std::thread(pipeThread, this);

    if (m_autoDeleteStreamPipeOnExit || detach)
        m_pipeThreadP.detach();

    return true;
}

void Bridge::sendPing()
{
    std::unique_lock<std::mutex> lock(m_endPingLoopMutex);
    using Ms = std::chrono::milliseconds;

    while(!m_isPingFinished)
    {
        if (m_endPingCond.wait_for(lock,Ms(m_pingEveryMS)) == std::cv_status::timeout)
        {
            // Send some ping...
            m_bridgeThreadPrc->sendPing();
        }
        else
        {
            // notified to terminate pings...
        }
    }
}

int Bridge::wait()
{
    m_pipeThreadP.join();
    return m_finishingPeer;
}

int Bridge::process()
{
    if (!m_peers[SIDE_FORWARD] || !m_peers[SIDE_BACKWARD])
        return -1;

    if (!m_bridgeThreadPrc)
    {
        m_bridgeThreadPrc = new Bridge_Thread();
        m_autoDeleteCustomPipeOnClose = true;
    }

    m_bridgeThreadPrc->setSocketEndpoints(m_peers[SIDE_BACKWARD],m_peers[SIDE_FORWARD], m_transmitionMode == TRANSMITION_MODE_CHUNKSANDPING);

    if (m_bridgeThreadPrc->startPipeSync())
    {
        m_isPingFinished = false;
        std::thread pingerThread;
        if (m_transmitionMode == TRANSMITION_MODE_CHUNKSANDPING)
        {
             pingerThread = std::thread(pingThread, this);
        }

        std::thread remotePeerThreadP = std::thread(remotePeerThread, this);
        processPeer(SIDE_FORWARD);
        remotePeerThreadP.join();

        // Wait until the ping loop is in "rest mode"
        if (m_transmitionMode == TRANSMITION_MODE_CHUNKSANDPING)
        {
            std::unique_lock<std::mutex> lock(m_endPingLoopMutex);
            // and notify that there is no more pings
            m_isPingFinished = true;
            m_endPingCond.notify_one();
        }
        if (m_transmitionMode == TRANSMITION_MODE_CHUNKSANDPING)
        {
            pingerThread.join();
        }

    }

    // All connections terminated.
    if (m_closeRemotePeerOnFinish)
    {
        // close them also.
        m_peers[1]->closeSocket();
        m_peers[0]->closeSocket();
    }

    if ( m_autoDeleteCustomPipeOnClose )
    {
        delete m_bridgeThreadPrc;
        m_bridgeThreadPrc = nullptr;
    }

    return m_finishingPeer;
}

bool Bridge::processPeer(Side currentSide)
{
    if (currentSide>1)
        return false;

    Side oppositeSide = currentSide==SIDE_FORWARD?SIDE_BACKWARD:SIDE_FORWARD;

    std::atomic<uint64_t> * bytesCounter = currentSide==SIDE_FORWARD?&m_sentBytes:&m_recvBytes;

    int dataRecv=1;
    while ( dataRecv > 0 )
    {
        dataRecv = m_bridgeThreadPrc->processPipe(currentSide);
        // >0 : data processed
        //  0 : ordered socket shutdown
        // -1 : socket error
        // -2: write error (don't need to close or do nothing, just bye because the other peer will report the read error in the same socket)
        // -3: ping
        if (dataRecv>0)
        {
            *bytesCounter+=dataRecv;
        }
        else if ( (dataRecv==-1 || dataRecv==0 ) && m_shutdownRemotePeerOnFinish )
        {
            m_lastError[oppositeSide] = dataRecv;
            m_peers[currentSide]->shutdownSocket();
            m_bridgeThreadPrc->terminate();
            m_finishingPeer = oppositeSide;
        }
        else if ( dataRecv==-3 ) // Only pings are received from one side.
        {
            std::unique_lock<std::mutex> l(m_lastPingMutex);
            m_lastPingTimestamp = time(nullptr);
            // set to continue to iterate:
            dataRecv = 1;
        }
    }

    return true;
}

bool Bridge::setPeer(Side i, std::shared_ptr<Socket_Stream> s)
{
    if (i>1)
        return false;
    m_peers[i] = s;
    return true;
}

std::shared_ptr<Socket_Stream> Bridge::getPeer(Side i)
{
    if (i>1)
        return nullptr;
    return m_peers[i];
}

void Bridge::setAutoDeleteStreamPipeOnThreadExit(bool value)
{
    m_autoDeleteStreamPipeOnExit = value;
}

void Bridge::setToShutdownRemotePeer(bool value)
{
    m_shutdownRemotePeerOnFinish = value;
}

void Bridge::setToCloseRemotePeer(bool value)
{
    m_closeRemotePeerOnFinish = value;
}

uint64_t Bridge::getSentBytes() const
{
    return m_sentBytes;
}

uint64_t Bridge::getRecvBytes() const
{
    return m_recvBytes;
}

bool Bridge::isAutoDeleteStreamPipeOnThreadExit() const
{
    return m_autoDeleteStreamPipeOnExit;
}
/*
bool Bridge::isAutoDeleteSocketsOnExit() const
{
    return autoDeleteSocketsOnExit;
}

void Bridge::setAutoDeleteSocketsOnExit(bool value)
{
    autoDeleteSocketsOnExit = value;
}
*/
// TODO: deleteOnExit
void Bridge::setCustomPipeProcessor(Bridge_Thread *value, bool deleteOnExit)
{
    m_bridgeThreadPrc = value;
}

Bridge::TransmitionMode Bridge::getTransmitionMode() const
{
    return m_transmitionMode;
}

void Bridge::setTransmitionMode(TransmitionMode newTransmitionMode)
{
    m_transmitionMode = newTransmitionMode;
}

time_t Bridge::getLastPing()
{
    std::unique_lock<std::mutex> l(m_lastPingMutex);
    return m_lastPingTimestamp;
}

int Bridge::getLastError(Side side)
{
    if (side>1)
        return -1;
    return m_lastError[side];
}
