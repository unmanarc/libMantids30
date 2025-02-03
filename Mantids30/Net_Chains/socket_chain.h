#pragma once

#include <Mantids30/Net_Sockets/socket_stream_base.h>
#include "socket_chain_protocolbase.h"

#include <memory>
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

namespace Mantids30 { namespace Network { namespace Sockets {

class Socket_Chain : public Socket_Stream_Base
{
public:
    Socket_Chain(std::shared_ptr<Socket_Stream_Base>  _baseSocket, bool _deleteBaseSocketOnExit = true);
    virtual ~Socket_Chain();

    /**
     * @brief addToChain Add the chain element to the socket chains...
     * @param chainElement chain element (should be deleted later)
     * @return true if successfully initialized
     */
    bool addToChain(ChainProtocols::Socket_Chain_ProtocolBase * chainElement, bool deleteAtExit = false);
    bool addToChain(std::pair<std::shared_ptr<Socket_Stream_Base> , std::shared_ptr<Socket_Stream_Base> > sockPairs,
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
     * @return pair of Socket_Stream_Base ptr
     */
    std::pair<std::shared_ptr<Socket_Stream_Base> , std::shared_ptr<Socket_Stream_Base> > getSocketPairLayer(size_t layer);

    ////////////////////
    // virtuals:
    bool isConnected() override;
    int shutdownSocket(int mode = SHUT_WR) override;
    int partialRead(void * data, const uint32_t & datalen) override;
    int partialWrite(const void * data, const uint32_t & datalen) override;

private:

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
        std::shared_ptr<Socket_Stream_Base>  sock[2];
        std::thread thr1,thr2;

        // Results from threads...
        int r0[2];
        bool w1[2];

        std::atomic<bool> detached, finished;
        bool deleteFirstSocketOnExit, deleteSecondSocketOnExit;
        bool modeServer;
    };


    struct sChainTElement {
        std::shared_ptr<Socket_Stream_Base>  sockets[2];
        int * r0;
        bool * w1;
        bool modeFWD;
    };

    static void chainThread(sChainTElement * chain);

    bool m_endPointReached;
    void removeSocketsOnExit();

    bool m_deleteBaseSocketOnExit;
    std::shared_ptr<Socket_Stream_Base> m_baseSocket;
    std::vector<sChainVectorItem *> m_socketLayers;
};

}}}

