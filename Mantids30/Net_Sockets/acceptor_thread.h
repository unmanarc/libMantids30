#pragma once

#include <memory>

#include "acceptor_callbacks.h"

namespace Mantids30::Network::Sockets::Acceptors {

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
    void setClientSocket(const std::shared_ptr<Sockets::Socket_Stream> &_clientSocket);

    /**
     * @brief getRemotePair Get Remote Host Address
     * @return remote pair null terminated string.
     */
    [[nodiscard]] std::string getRemotePair() const;

    [[nodiscard]] uint16_t getLocalPort();

    static void thread_streamclient(const std::shared_ptr<StreamAcceptorThread> &threadClient, void *threadedAcceptedControl);

private:
    std::shared_ptr<Sockets::Socket_Stream> m_pClientSocket;
};

} // namespace Mantids30::Network::Sockets::Acceptors
