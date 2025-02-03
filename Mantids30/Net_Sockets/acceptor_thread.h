#pragma once

#include <memory>

#include "acceptor_callbacks.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace Acceptors {


/**
 * Class for managing the client on his thread.
 */
class StreamAcceptorThread
{
public:
    /**
     * constructor
     */
    StreamAcceptorThread() = default;
    /**
     * destructor
     */
    ~StreamAcceptorThread() = default;

    StreamAcceptorThreadCallbacks callbacks;

    /**
     * Kill the client socket
     */
    void stopSocket();
    /**
     * Call callback
     * to be used from the client thread.
     */
    void postInitConnection();
    /**
     * Set socket
     */
    void setClientSocket(std::shared_ptr<Sockets::Socket_Stream_Base> _clientSocket);

    /**
     * @brief getRemotePair Get Remote Host Address
     * @return remote pair null terminated string.
     */
    std::string getRemotePair() const;

    static void thread_streamclient(std::shared_ptr<StreamAcceptorThread> threadClient, void *threadedAcceptedControl);

private:


    std::shared_ptr<Sockets::Socket_Stream_Base> m_pClientSocket;

};

}}}}


