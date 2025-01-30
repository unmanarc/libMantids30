#pragma once

#include "socket_stream_base.h"
#include <memory>

namespace Mantids30 { namespace Network { namespace Sockets { namespace Acceptors {

/**
 * Class for managing the client on his thread.
 */
class SAThread
{
public:
    /**
     * constructor
     */
    SAThread();
    /**
     * destructor
     */
    ~SAThread();
    // /**
    //  * Start the thread of the client.
    //  */
    // void start();
    // Use the thread with the thread_streamclient
    /**
     * Kill the client socket
     */
    void stopSocket();
    ///**
    // * Set parent (stream acceptor object)
    // * @param parent parent
    // */
    //void setParent(void *parent);
    /**
     * Set callback when connection is fully established (if your function returns false, the socket will not be destroyed by this)
     */
    void setCallbackOnConnect(bool (*_onConnect)(std::shared_ptr<void> , std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool), std::shared_ptr<void> contextOnConnected);
    /**
     * Set callback when protocol initialization failed (like bad X.509 on TLS)
     */
    void setCallbackOnInitFail(bool (*_onInitFailed)(std::shared_ptr<void>, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool), std::shared_ptr<void> contextOnInitFailed);
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
    const char * getRemotePair();

    bool isSecure() const;
    void setSecure(bool value);

    static void thread_streamclient(std::shared_ptr<SAThread> threadClient, void *threadedAcceptedControl);

private:


    std::shared_ptr<Sockets::Socket_Stream_Base> m_pClientSocket;
    bool (*m_onConnect)(std::shared_ptr<void> ,std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);
    bool (*m_onInitFail)(std::shared_ptr<void> ,std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);

    char m_remotePair[INET6_ADDRSTRLEN];
    bool m_isSecure;

    std::shared_ptr<void> m_contextOnConnect = nullptr;
    std::shared_ptr<void> m_contextOnInitFail = nullptr;
  //  void * m_parent;
};

}}}}


