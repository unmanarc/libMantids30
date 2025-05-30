#pragma once

#include <mutex>
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

// default values to prevent network error application hangs (because not everybody supports tcp keepalives)...
#define DEFAULT_SOCKRW_TIMEOUT 60*5

namespace Mantids30 { namespace Network { namespace Sockets {

class Socket_TCP;
/**
 * Socket base class
 * Manipulates all kind of sockets (udp,tcp,unix, etc)
 */
class Socket {
    friend class Socket_TCP;
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors/Destructors/Copy:
    /**
     * Socket Class Constructor
     * note: does not initialize the socket
     */
    Socket();
    virtual ~Socket();

    //Keep the copy constructor and assignment operator as deleted or private to prevent duplication.
    Socket(const Socket &) = delete;
    Socket& operator=(Socket) = delete;

#ifdef _WIN32
    static bool win32Init();
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Socket Options:
    /**
     * Set to use write
     * use write instead send.
     */
    void setUseWriteInsteadRecv();
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
     * @brief Get the last bind address and port in normalized string format.
     * @return A string in the format "IP:Port".
     */
    std::string getLastBindAddress() const;

    /**
     * @brief getRemotePairStr Non-Efficient get remote pair string (useful only for cheap print operations)
     * @return remote pair as string.
     */
    std::string getRemotePairStr();
    /**
     * Get remote pair address
     * @param address pair address char * (should contain at least 64 bytes)
     */
    void getRemotePair(char * address) const;

    /**
     * @brief getRemotePort Get Remote Port for listening connections
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
     * @brief getSocketOption Get Socket Option
     * @param level SOL_SOCKET / IPPROTO_TCP / ...
     * @param optname SO Option Name
     * @param optval Option Value Pointer
     * @param optlen IN/OUT option Value Memory Length
     * @return zero if succeed. error -1
     */
    int getSocketOption(int level, int optname, void *optval, socklen_t *optlen);
    /**
     * @brief setSocketOption Set Socket Option
     * @param level SOL_SOCKET / IPPROTO_TCP / ...
     * @param optname SO Option Name
     * @param optval Option Value Pointer
     * @param optlen Option Value Memory Length
     * @return zero if succeed. error -1
     */
    int setSocketOption(int level, int optname, const void *optval, socklen_t optlen);
    /**
     * @brief setSocketOptionBool Set Socket Option as bool
     * @param level SOL_SOCKET / IPPROTO_TCP / ...
     * @param optname SO Option Name
     * @param value value as true or false
     * @return zero if succeed. error -1
     */
    int setSocketOptionBool(int level, int optname, bool value);

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

    // Real/Pure socket shutdown:
    int _shutdownSocket(int mode = SHUT_RDWR);

    /**
     * Read a data block from the socket
     * Receive the data block in only one command (without chunks).
     * note that this haves some limitations. some systems can only receive 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual ssize_t partialRead(void * data, const uint32_t & datalen);
    /**
     * Write a data block to the socket
     * note that this haves some limitations. some systems can only send 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual ssize_t partialWrite(const void *data, const uint32_t & datalen);

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

    /**
     * @brief Retrieve the name associated with this connection.
     *
     * This name is used to uniquely identify a connection. It can be a random string,
     * a mnemonic name, or any identifier that helps distinguish this socket connection
     * from others. For example, it could be a randomly generated key or a human-readable
     * identifier.
     *
     * @return The name associated with this connection.
     */
    std::string getConnectionName() const;

    /**
     * @brief Set a name for this connection.
     *
     * Assigns a custom name or identifier to this socket connection. This can be useful
     * for logging, debugging, or tracking specific connections. The value can be
     * anything meaningful, such as a random key, a session identifier, or a descriptive
     * label for the connection.
     *
     * @param newConnectionName The name or identifier to associate with this connection.
     */
    void setConnectionName(const std::string &newConnectionName);


private:
    static void socketSystemInitialization();
    //void initVars();

protected:
    /**
     * Set remote pair address
     * Used by internal functions...
     * @param address pair address char * (should contain at least 64 bytes)
     */
    void setRemotePair(const char * address);

    bool bindTo(const char * bindAddress = nullptr, const uint16_t &port = 0);
    bool getAddrInfo(const char *remoteHost, const uint16_t &remotePort, int ai_socktype, void ** res);

    bool m_useIPv6 = false;
    /**
     * if true, Use write instead send and read instead recv.
     */
    bool m_useWriteInsteadRecv = false;
    /**
     * last error message.
     */
    std::string m_lastError;
    /**
     * buffer with the remote pair address.
     */
    char m_remotePair[INET6_ADDRSTRLEN] = "";
    /**
     * @brief remotePort remote port when accepting connections.
     */
    unsigned short m_remotePort = 0;
    /**
     * @brief m_remoteServerHostname The server hostname configured when connecting...
     */
    std::string m_remoteServerHostname;

    // Store the last bind information as normalized structs
    struct sockaddr_in m_lastBindIPv4;
    struct sockaddr_in6 m_lastBindIPv6;
    // Keep track of whether there was a successful bind
    bool m_lastBindValid = false;

    static bool m_globalSocketInitialized;

    std::atomic<unsigned int> m_readTimeout = {DEFAULT_SOCKRW_TIMEOUT};
    std::atomic<unsigned int> m_writeTimeout = {DEFAULT_SOCKRW_TIMEOUT};
    std::atomic<unsigned int> m_recvBuffer = {0};

    std::string m_connectionName;

    /**
     * @brief m_isInListenMode The socket is in listen mode.
     */
    bool m_isInListenMode = false;

    /**
     * @brief sockfd Socket descriptor
     */
    std::atomic<int> m_sockFD = -1;

    std::mutex mutexClose;


    bool m_shutdownProtocolOnRead = false;
    bool m_shutdownProtocolOnWrite = false;

#ifdef _WIN32
    static bool m_isWinSockInitialized;
#endif
};

typedef std::shared_ptr<Socket> Socket_SP;

}}}

