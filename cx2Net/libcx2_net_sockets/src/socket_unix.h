#ifndef SOCKET_UNIX_H
#define SOCKET_UNIX_H

#include "streamsocket.h"

namespace CX2 { namespace Network { namespace Sockets {


/**
 * Unix Socket Class
 */
class  Socket_UNIX : public Streams::StreamSocket {
public:
	/**
	 * Class constructor.
	 */
    Socket_UNIX();
    /**
     * Listen on an specific path and address
     * @param listenOnAddress listening path
     * @param port unused parameter.
     * @return true if we can bind to that path.
     */
    bool listenOn(const uint16_t & port, const char * listenOnAddr, bool useIPv4 = true, const int32_t & recvbuffer = 0, const int32_t &backlog = 10) override;
    /**
     * Connect to remote host using an UNIX socket.
     * @param path local path to connect to.
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection.
     * @return true if successfully connected
     */
    bool connectTo(const char * path, const uint16_t &, const uint32_t & timeout = 30) override;
    /**
     * Accept a new connection on a listening socket.
     * @return returns a socket with the new connection.
     */
    StreamSocket *acceptConnection() override;
};

typedef std::shared_ptr<Socket_UNIX> Socket_UNIX_SP;
}}}
#endif // SOCKET_UNIX_H
