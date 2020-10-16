#ifndef SOCKET_TCP_H
#define SOCKET_TCP_H

#include "streamsocket.h"
#include <unistd.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

namespace CX2 { namespace Network { namespace Sockets {

/**
 * TCP Socket Class
 */
class Socket_TCP : public Streams::StreamSocket {
public:
    /**
     * Class constructor.
     */
    Socket_TCP();
    virtual ~Socket_TCP();
    /**
     * Listen on an specific TCP port and address
     * @param listenOnAddress address to listen on. (use * for any address)
     * @param port 16-bit unsigned integer with the listening TCP port (1-65535), 0 means random available port.
     * @return true if the operation succeeded.
     */
    bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t &recvbuffer = 0, const int32_t &backlog = 10) override;
    /**
     * Connect to remote host using a TCP socket.
     * @param remoteHost remote hostname to connect to, can be the hostname or the ip address
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection. (default 30)
     * @return true if successfully connected
     */
    bool connectTo(const char * bindAddress, const char * remoteHost, const uint16_t & port, const uint32_t & timeout = 30) override;
    /**
     * Accept a new TCP connection on a listening socket.
     * @return returns a socket with the new established tcp connection.
     */
    virtual StreamSocket *acceptConnection() override;

    /**
     * Virtual function for protocol initialization after the connection starts...
     * useful for SSL server, it runs in blocking mode and should be called apart to avoid tcp accept while block
     * @return returns true if was properly initialized.
     */
    virtual bool postAcceptSubInitialization() override;

    int setTCPOptionBool(const int32_t & optname, bool value = true);
    int setTCPOption(const int32_t & optname,const void *optval, socklen_t optlen);
    int getTCPOption(const int32_t &optname, void *optval, socklen_t * optlen);

    void overrideReadTimeout(int32_t tout = -1);
    void overrideWriteTimeout(int32_t tout = -1);

    virtual bool isSecure() override;

protected:

private:
    bool tcpConnect(const struct sockaddr *addr, socklen_t addrlen, uint32_t timeout);

    int32_t ovrReadTimeout,ovrWriteTimeout;
};

typedef std::shared_ptr<Socket_TCP> Socket_TCP_SP;

}}}

#endif // SOCKET_TCP_H
