#ifndef VSTREAMTHREAD_H
#define VSTREAMTHREAD_H

#include <thread>
#include "streamsocket.h"

namespace CX2 { namespace Network { namespace Sockets { namespace Acceptors {

/**
 * Class for managing the client on his thread.
 */
class Socket_Acceptor_Thread
{
public:
    /**
     * constructor
     */
    Socket_Acceptor_Thread();
    /**
     * destructor
     */
    ~Socket_Acceptor_Thread();
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
     * Set callback when connection is fully established
     */
    void setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *, bool), void *objOnConnected);
    /**
     * Set callback when protocol initialization failed (like bad X.509 on TLS)
     */
    void setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *, bool), void *objOnConnected);
    /**
     * Call callback
     * to be used from the client thread.
     */
    void postInitConnection();
    /**
     * Set socket
     */
    void setClientSocket(Streams::StreamSocket * _clientSocket);

    /**
     * @brief getRemotePair Get Remote Host Address
     * @return remote pair null terminated string.
     */
    const char * getRemotePair();

    bool getIsSecure() const;
    void setIsSecure(bool value);

private:
    static void thread_streamclient(Socket_Acceptor_Thread * threadClient, void * threadedAcceptedControl);

    Streams::StreamSocket * clientSocket;
    bool (*callbackOnConnect)(void *,Streams::StreamSocket *, const char *, bool);
    bool (*callbackOnInitFail)(void *,Streams::StreamSocket *, const char *, bool);

    char remotePair[INET6_ADDRSTRLEN+2];
    bool isSecure;

    void *objOnConnect, *objOnInitFail;
    void * parent;
};

}}}}


#endif // VSTREAMTHREAD_H
