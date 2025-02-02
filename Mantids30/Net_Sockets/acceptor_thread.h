#pragma once

#include <memory>

#include "acceptor_callbacks.h"

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


    SAThreadCallbacks callbacks;


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

    char m_remotePair[INET6_ADDRSTRLEN];
    bool m_isSecure;

  //  void * m_parent;
};

}}}}


