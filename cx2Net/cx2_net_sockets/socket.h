#ifndef SOCKET_H
#define SOCKET_H

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <ws2tcpip.h>
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#endif

#include <stdint.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <atomic>

namespace CX2 { namespace Network { namespace Sockets {

enum SocketMode
{
    DATAGRAM_SOCKET,
    STREAM_SOCKET,
    UNINITIALIZED_SOCKET
};

/**
 * Socket base class
 * Manipulates all kind of sockets (udp,tcp,unix, etc)
 */
class Socket {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors/Destructors/Copy:
    /**
     * Socket Class Constructor
     * note: does not initialize the socket
     */
    Socket();
    virtual ~Socket();

    Socket(const Socket &);
    Socket& operator=(Socket);

#ifdef _WIN32
    static bool win32Init();
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Options:
    /**
     * Set to use write
     * use write instead send.
     */
    void setUseWrite();
    /**
     * Set Read timeout.
     * @param _timeout timeout in seconds
     */
    bool setReadTimeout(unsigned int _timeout);
    /**
     * Set Write timeout.
     * @param _timeout timeout in seconds
     */
    bool setWriteTimeout(unsigned int _timeout);
    /**
     * Set system buffer size.
     * Use to increase the current reception buffer
     * @param buffsize buffer size in bytes.
     */
    void setRecvBuffer(int buffsize);
    /**
     * @brief setBlockingMode Set Socket Blocking Mode
     * @param blocking mode (false: non-blocking)
     * @return true if succeeed
     */
    bool setBlockingMode(bool blocking = true);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Manipulation:
    /**
     * Set socket file descriptor value.
     * This is useful to manipulate already created sockets.
     * @param _sockfd socket file descriptor
     */
    void setSocketFD(int _sockfd);

    /**
     * Get Current Socket file descriptor, and leave this object without one. (will not close)
     * @return socket file descriptor
     */
    int adquireSocketFD();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Status:
    /**
     * Check if we have an initialized socket.
     * @return true if the socket is a valid file descriptor
     */
    bool isActive() const;
    /**
     * Check if the remote pair is connected or not.
     * @param true if is it connected
     */
    virtual bool isConnected();
    /**
     * Get current used port.
     * Useful for TCP/UDP connections, especially on received connections.
     * @param 16-bit unsigned integer with the port (0-65535)
     */
    uint16_t getPort();
    /**
     * Get last error message
     * @param last error message pointer. (static mem)
     */
    std::string getLastError() const;
    /**
     * Get remote pair address
     * @param address pair address char * (should contain at least 64 bytes)
     */
    void getRemotePair(char * address) const;
    /**
     * Set remote pair address
     * Used by internal functions...
     * @param address pair address char * (should contain at least 64 bytes)
     */
    void setRemotePair(const char * address);
    /**
     * @brief getRemotePort Get Remote Port for listenning connections
     * @return remote port 0-65535
     */
    unsigned short getRemotePort() const;
    /**
     * Set remote port
     * Used by internal functions...
     * @param value port
     */
    void setRemotePort(unsigned short value);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Options...
    /**
     * @brief getSockOpt Get Socket Option
     * @param level IP / TCP / ...
     * @param optname SO Option Name
     * @param optval Option Value Pointer
     * @param optlen IN/OUT option Value Memory Length
     * @return zero if succeed. error -1
     */
    int getSockOpt(int level, int optname, void *optval, socklen_t *optlen);
    /**
     * @brief setSockOpt Set Socket Option
     * @param level IP / TCP / ...
     * @param optname SO Option Name
     * @param optval Option Value Pointer
     * @param optlen Option Value Memory Length
     * @return zero if succeed. error -1
     */
    int setSockOpt(int level, int optname, const void *optval, socklen_t optlen);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Connection elements...
    /**
     * Connect to remote host
     * This is a virtual function
     * @param remoteHost remote hostname to connect to
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection.
     * @return true if successfully connected
     */
    bool connectTo(const char * remoteHost, const  uint16_t & port, const uint32_t & timeout = 30);
    /**
     * @brief connectFrom Connect to remote host using a local bind address (eg. multi-homed network)
     * @param localBindAddress address to bind, if empty, use default.
     * @param remoteHost remote hostname to connect to
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection.
     * @return true if successfully connected
     */
    virtual bool connectFrom(const char * localBindAddress, const char * remoteHost, const  uint16_t & port, const uint32_t & timeout = 30);
    /**
     * Try connect until it succeeds.
     */
    void tryConnect(const char * remoteHost, const uint16_t & port, const uint32_t & timeout);
    /**
     * Bind and Listen on an specific port and address (does not require to bind)
     * @param listenOnAddress address to listen on. (use :: for ipv6 or 0.0.0.0 if ipv4)
     * @param port 16-bit unsigned integer with the listening port (0-65535)
     * @return true if we can bind the port.
     */
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t & recvbuffer = 0, const int32_t &backlog = 10);


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Connection Finalization...
    /**
     * Close the socket itself (will close the connection)
     */
    int closeSocket();
    /**
     * Shutdown the internal protocol and connection
     * Use for terminate the connection. both sides will start to fail to receive/send
     * You should also use Close too. (or reuse the socket?)
     */
    virtual int shutdownSocket(int mode = SHUT_RDWR);
    /**
     * Read a data block from the socket
     * Receive the data block in only one command (without chunks).
     * note that this haves some limitations. some systems can only receive 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual int partialRead(void * data, const uint32_t & datalen);
    /**
     * Write a data block to the socket
     * note that this haves some limitations. some systems can only send 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual int partialWrite(const void *data, const uint32_t & datalen);

    /**
     * @brief iShutdown Internal protocol Shutdown
     * @return depends on protocol.
     */
    virtual int iShutdown(int mode = SHUT_RDWR);

    virtual bool isSecure() { return false; }

    /**
     * @brief getUseIPv6 Get if using IPv6 Functions
     * @return true if using IPv6
     */
    bool getUseIPv6() const;
    /**
     * @brief setUseIPv6 Set socket to use IPv6 (default: false), must be called before connect/listen functions.
     * note: some protocols like unix sockets does not use ipv6 at all.
     * @param value true if want to use IPv6
     */
    void setUseIPv6(bool value);

private:
    static void socketSystemInitialization();
    void initVars();

protected:
    bool bindTo(const char * bindAddress = nullptr, const uint16_t &port = 0);
    bool getAddrInfo(const char *remoteHost, const uint16_t &remotePort, int ai_socktype, void ** res);

    bool useIPv6;
    /**
     * if true, Use write instead send and read instead recv.
     */
    bool useWrite;
    /**
     * last error message.
     */
    std::string lastError;
    /**
     * buffer with the remote pair address.
     */
    char remotePair[INET6_ADDRSTRLEN];
    /**
     * @brief remotePort remote port when accepting connections.
     */
    unsigned short remotePort;

    static bool socketInitialized;
    static bool badSocket;

    std::atomic<unsigned int> readTimeout;
    std::atomic<unsigned int> writeTimeout;
    std::atomic<unsigned int> recvBuffer;

    /**
     * @brief listenMode The socket is in listen mode.
     */
    bool listenMode;


    /**
     * @brief sockfd Socket descriptor
     */
    int sockfd;


    bool shutdown_proto_rd;
    bool shutdown_proto_wr;


    bool shutdown_rd;
    bool shutdown_wr;

#ifdef _WIN32
    static bool winSockInitialized;
#endif
};

typedef std::shared_ptr<Socket> Socket_SP;

}}}

#endif // SOCKET_H
