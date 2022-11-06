#ifndef SOCKET_BRIDGE
#define SOCKET_BRIDGE

#include "socket_streambase.h"
#include "streams_bridge_thread.h"

#include <cstdint>
#include <stdint.h>

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace Mantids { namespace Network { namespace Sockets { namespace NetStreams {

/**
 * @brief The Bridge class connect two pipe stream sockets.
 */
class Bridge
{
public:
    enum TransmitionMode {
        TRANSMITION_MODE_STREAM=0,
        TRANSMITION_MODE_CHUNKSANDPING=1
    };
    /**
     * @brief Socket_Bridge constructor.
     */
    Bridge();
    /**
     * @brief Socket_Bridge destructor.
     */
    ~Bridge();
    /**
     * @brief start, begin the communication between peers in threaded mode.
     * @param autoDelete true (default) if going to delete the whole pipe when finish.
     * @return true if initialized, false if not.
     */
    bool start(bool _autoDeleteStreamPipeOnExit = true, bool detach = true);
    /**
     * @brief wait will block-wait until thread finishes
     * @return -1 failed, 0: socket 0 closed the connection, 1: socket 1 closed the connection.
     */
    int wait();

    /**
     * @brief process, begin the communication between peers blocking until it ends.
     * @return -1 failed, 0: socket 0 closed the connection, 1: socket 1 closed the connection.
     */
    int process();
    /**
     * @brief processPeer, begin the communication between peer i to the next peer.
     * @param i peer number (0 or 1)
     * @return true if transmitted and closed, false if failed.
     */
    bool processPeer(unsigned char i);
    /**
     * @brief SetPeer Set Stream Socket Peer (0 or 1)
     * @param i peer number: 0 or 1.
     * @param s peer established socket.
     * @return true if peer setted successfully.
     */
    bool setPeer(unsigned char i, Sockets::Socket_StreamBase * s, const TransmitionMode & tm = TRANSMITION_MODE_STREAM);
    /**
     * @brief GetPeer Get the Pipe Peers
     * @param i peer number (0 or 1)
     * @return Stream Socket Peer.
     */
    Sockets::Socket_StreamBase * getPeer(unsigned char i);
    /**
     * @brief setAutoDelete Auto Delete the pipe object when finish threaded job.
     * @param value true for autodelete (default), false for not.
     */
    void setAutoDeleteStreamPipeOnThreadExit(bool value = true);
    /**
     * @brief shutdownRemotePeer set to shutdown both sockets peer on finish.
     * @param value true for close the remote peer (default), false for not.
     */
    void setToShutdownRemotePeer(bool value = true);
    /**
     * @brief closeRemotePeer set to close both sockets peer on finish.
     * @param value true for close the remote peer (default), false for not.
     */
    void setToCloseRemotePeer(bool value = true);
    /**
     * @brief getSentBytes Get bytes transmitted from peer 0 to peer 1.
     * @return bytes transmitted.
     */
    uint64_t getSentBytes() const;
    /**
     * @brief getRecvBytes Get bytes  transmitted from peer 1 to peer 0.
     * @return bytes transmitted.
     */
    uint64_t getRecvBytes() const;
    /**
     * @brief isAutoDeleteStreamPipeOnThreadExit Get if this class autodeletes when pipe is over.
     * @return true if autodelete is on.
     */
    bool isAutoDeleteStreamPipeOnThreadExit() const;
    /**
     * @brief isAutoDeleteSocketsOnExit Get if pipe endpoint sockets are going to be deleted when this class is destroyed.
     * @return true if it's going to be deleted.
     */
    bool isAutoDeleteSocketsOnExit() const;
    /**
     * @brief setAutoDeleteSocketsOnExit Set if pipe endpoint sockets are going to be deleted when this class is destroyed.
     * @param value true if you want sockets to be deleted on exit.
     */
    void setAutoDeleteSocketsOnExit(bool value);
    /**
     * @brief setCustomPipeProcessor Set custom pipe processor.
     * @param value pipe processor.
     */
    void setCustomPipeProcessor(Bridge_Thread *value, bool deleteOnExit = false);

    /**
     * @brief getTransmitionMode Get Transmition Mode
     * @return value with the mode (chunked or streamed)
     */
    TransmitionMode getTransmitionMode() const;
    /**
     * @brief setTransmitionMode set the transmition mode
     * @param newTransmitionMode  value with the mode (chunked or streamed)
     */
    void setTransmitionMode(TransmitionMode newTransmitionMode);


    /**
     * @brief sendPing
     */
    void sendPing();


    uint32_t getPingEveryMS() const;
    void setPingEveryMS(uint32_t newPingEveryMS);

private:
    static void remotePeerThread(Bridge * stp);
    static void pingThread(Bridge * stp);
    static void pipeThread(Bridge * stp);

    Bridge_Thread *bridgeThreadPrc;

    Sockets::Socket_StreamBase * peers[2];
    TransmitionMode transmitionMode;

    std::atomic<uint64_t> sentBytes,recvBytes;
    std::atomic<int> finishingPeer;
    std::atomic<bool> shutdownRemotePeerOnFinish;
    std::atomic<bool> closeRemotePeerOnFinish;

    std::mutex mutex_endPingLoop;
    std::condition_variable cond_endPing;

    uint32_t pingEveryMS;

    bool pingFinished;
    bool autoDeleteStreamPipeOnExit;
    bool autoDeleteSocketsOnExit;
    bool autoDeleteCustomPipeOnClose;

    std::thread pipeThreadP;
};

}}}}
#endif // SOCKET_BRIDGE
