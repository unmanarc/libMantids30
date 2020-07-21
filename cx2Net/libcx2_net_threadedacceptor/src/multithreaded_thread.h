#ifndef VSTREAMTHREAD_H
#define VSTREAMTHREAD_H

#include <thread>
#include <cx2_net_sockets/streamsocket.h>

namespace CX2 { namespace Network { namespace Streams { namespace ThreadedAcceptors {

/**
 * Class for managing the client on his thread.
 */
class MultiThreaded_Accepted_Thread
{
public:
    /**
     * constructor
     */
    MultiThreaded_Accepted_Thread();
    /**
     * destructor
     */
    ~MultiThreaded_Accepted_Thread();
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
    void setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *), void *objOnConnected);
    /**
     * Set callback when protocol initialization failed (like bad X.509 on TLS)
     */
    void setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *), void *objOnConnected);
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

private:
    static void thread_streamclient(MultiThreaded_Accepted_Thread * threadClient, void * threadedAcceptedControl);


    Streams::StreamSocket * clientSocket;
    bool (*callbackOnConnect)(void *,Streams::StreamSocket *, const char *);
    bool (*callbackOnInitFail)(void *,Streams::StreamSocket *, const char *);

    char remotePair[INET6_ADDRSTRLEN+2];

    void *objOnConnect, *objOnInitFail;
    void * parent;
};

}}}}


#endif // VSTREAMTHREAD_H
