#ifndef CHAINSOCKETS_H
#define CHAINSOCKETS_H

#include <mdz_net_sockets/streamsocket.h>
#include "socketchainbase.h"

#include <vector>
#include <atomic>
#include <utility>
#include <thread>

/*
  Example:

  read() <--------*                           2-                           2-                           2-
                   \                        THREAD                       THREAD                       THREAD
                    \ pair(sock[0],sock[1]) <----> pair(sock[0],sock[1]) <----> pair(sock[0],sock[1]) <----> baseSocket (O/S Network)
                    /
                   /
 write() -------->*

*/

namespace Mantids { namespace Network { namespace Chains {

struct sChainVectorItem {
    sChainVectorItem()
    {
        r0[0]=0;
        w1[0]=true;
        r0[1]=0;
        w1[1]=true;
        detached = false;
        finished = false;
    }

    /**
     * @brief sock connected pair sockets (sock[0]: up socket  sock[1]: down socket)
     */
    Mantids::Network::Streams::StreamSocket * sock[2];
    std::thread thr1,thr2;

    // Results from threads...
    int r0[2];
    bool w1[2];

    std::atomic<bool> detached, finished;
    bool deleteFirstSocketOnExit, deleteSecondSocketOnExit;
    bool modeServer;
};

struct sChainTElement {
    Mantids::Network::Streams::StreamSocket * sockets[2];
    int * r0;
    bool * w1;
    bool modeFWD;
};

class ChainSockets : public Mantids::Network::Streams::StreamSocket
{
public:
    ChainSockets(Mantids::Network::Streams::StreamSocket * _baseSocket, bool _deleteBaseSocketOnExit = true);
    virtual ~ChainSockets();

    /**
     * @brief addToChain Add the chain element to the socket chains...
     * @param chainElement chain element (should be deleted later)
     * @return true if successfully initialized
     */
    bool addToChain(SocketChainBase * chainElement);
    bool addToChain(std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket *> sockPairs,
                    bool deleteFirstSocketOnExit = false,
                    bool deleteSecondSocketOnExit = true,
                    bool modeServer = false,
                    bool detached = false,
                    bool endPMode = false);
    void waitUntilFinish();

    ////////////////////
    // errors:
    /**
     * @brief getLayers Get number of layers
     * @return number of layers.
     */
    size_t getLayers();
    /**
     * @brief getLayerReadResultValue Read thread last read error. (don't use before waitUntilFinish)
     * @param layer Layer number [0..n-1]
     * @param fwd true: sock[1]->baseSocket, false: baseSocket->sock[1]
     * @return socket last read error. (0 shutdown, -1 error, -2 layer does not exist)
     */
    int getLayerReadResultValue(size_t layer, bool fwd);
    /**
     * @brief getLayerWriteResultValue Read thread last write error. (don't use before waitUntilFinish)
     * @param layer Layer number [0..n-1]
     * @param fwd true: sock[1]->baseSocket, false: baseSocket->sock[1]
     * @return socket last write error. (false: was not able to write on next socket, true: was able to write on next socket)
     */
    bool getLayerWriteResultValue(size_t layer, bool fwd);
    /**
     * @brief getSocketPairLayer Get Sockets Pair from layer (don't use with )
     * @param layer layer number [0..n-1]
     * @return pair of Mantids::Network::Streams::StreamSocket ptr
     */
    std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket *> getSocketPairLayer(size_t layer);

    ////////////////////
    // virtuals:
    int shutdownSocket(int mode = SHUT_WR) override;
    int partialRead(void * data, const uint32_t & datalen) override;
    int partialWrite(const void * data, const uint32_t & datalen) override;

private:
    static void chainThread(sChainTElement * chain);



    bool endPointReached;
    void removeSocketsOnExit();

    bool deleteBaseSocketOnExit;
    Mantids::Network::Streams::StreamSocket * baseSocket;
    std::vector<sChainVectorItem *> socketLayers;
};

}}}

#endif // CHAINSOCKETS_H
