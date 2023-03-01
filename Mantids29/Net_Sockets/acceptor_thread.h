#ifndef SATHREAD_H
#define SATHREAD_H

#include <thread>
#include "socket_stream_base.h"

namespace Mantids29 { namespace Network { namespace Sockets { namespace Acceptors {

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
    /**
     * Start the thread of the client.
     */
    void start();
    /**
     * Kill the client socket
     */
    void stopSocket();
    /**
     * Set parent (stream acceptor object)
     * @param parent parent
     */
    void setParent(void * parent);
    /**
     * Set callback when connection is fully established (if your function returns false, the socket will not be destroyed by this)
     */
    void setCallbackOnConnect(bool (*_onConnect)(void *, Sockets::Socket_Stream_Base *, const char *, bool), void *objectOnConnected);
    /**
     * Set callback when protocol initialization failed (like bad X.509 on TLS)
     */
    void setCallbackOnInitFail(bool (*_onInitFailed)(void *, Sockets::Socket_Stream_Base *, const char *, bool), void *objectOnInitFailed);
    /**
     * Call callback
     * to be used from the client thread.
     */
    void postInitConnection();
    /**
     * Set socket
     */
    void setClientSocket(Sockets::Socket_Stream_Base * _clientSocket);

    /**
     * @brief getRemotePair Get Remote Host Address
     * @return remote pair null terminated string.
     */
    const char * getRemotePair();

    bool isSecure() const;
    void setSecure(bool value);

private:
    static void thread_streamclient(SAThread * threadClient, void * threadedAcceptedControl);

    Sockets::Socket_Stream_Base * m_pClientSocket;
    bool (*m_onConnect)(void *,Sockets::Socket_Stream_Base *, const char *, bool);
    bool (*m_onInitFail)(void *,Sockets::Socket_Stream_Base *, const char *, bool);

    char m_remotePair[INET6_ADDRSTRLEN];
    bool m_isSecure;

    void *m_objectOnConnect, *m_objectOnInitFail;
    void * m_parent;
};

}}}}


#endif // SATHREAD_H
