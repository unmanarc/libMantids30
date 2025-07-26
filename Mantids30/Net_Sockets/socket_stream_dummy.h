#pragma once

/**
 * @file socket_stream_dummy.h
 * @brief Header file for Socket_Stream_Dummy class.
 */


#include <Mantids30/Memory/b_chunks.h>
#include "socket_stream.h"

namespace Mantids30 {
namespace Network {
namespace Sockets {
/**
 * @class Socket_Stream_Dummy
 * @brief Dummy socket stream implementation.
 *
 * This class provides a dummy socket stream implementation for testing and development purposes.
 */
class Socket_Stream_Dummy : public Mantids30::Network::Sockets::Socket_Stream
{
public:
    /**
     * @brief Constructor for Socket_Stream_Dummy.
     */
    Socket_Stream_Dummy() = default;

    /**
     * @brief Get the peer name of the socket.
     *
     * @return The peer name as a string.
     */
    std::string getPeerName() const override;

    /**
     * @brief Set the peer name for the socket.
     *
     * @param newPeerName The new peer name to set.
     */
    void setPeerName(const std::string &newPeerName);

    /**
     * @brief Check if the socket is secure.
     *
     * @return True if the socket is secure, false otherwise. In this dummy implementation, it always returns true.
     */
    bool isSecure() override;

    /**
     * @brief Check if the socket is connected.
     *
     * @return True if the socket is connected, false otherwise. In this dummy implementation, it always returns true.
     */
    bool isConnected() override;

    /**
     * @brief Shutdown the socket.
     *
     * @param mode The shutdown mode (SHUT_RD, SHUT_WR, or SHUT_RDWR).
     * @return 0 on success, -1 on error.
     */
    int shutdownSocket(int mode = SHUT_RDWR) override;

    /**
     * @brief Get the sender buffer chunk container.
     *
     * @return A pointer to the sender B_Chunks object.
     */
    Memory::Containers::B_Chunks *getSender();

    /**
     * @brief Get the receiver buffer chunk container.
     *
     * @return A pointer to the receiver B_Chunks object.
     */
    Memory::Containers::B_Chunks *getReceiver();

    /**
     * @brief Read a data block from the chunk container.
     *
     * @param data Pointer to the buffer where the read data will be stored.
     * @param datalen Maximum number of bytes to read.
     * @return Number of bytes read, 0 for end of file, or -1 on error.
     */
    ssize_t partialRead(void *data, const size_t &datalen) override;

    /**
     * @brief Write a data block to the chunk container.
     *
     * @param data Pointer to the buffer containing the data to write.
     * @param datalen Number of bytes to write.
     * @return Number of bytes written, 0 for end of file, or -1 on error.
     */
    ssize_t partialWrite(const void *data, const size_t &datalen) override;

    /**
     * @brief Override the remote pair address.
     *
     * This function allows overriding the remote pair address with a new one specified by \p address.
     *
     * @param address The new remote pair address as a C-style string (null-terminated).
     */
    void setRemotePairOverride(const char *address);


private:
    std::string peerName; /**< Peer name associated with the socket. */
    Memory::Containers::B_Chunks sender;   /**< Sender buffer chunk container. */
    Memory::Containers::B_Chunks receiver; /**< Receiver buffer chunk container. */
    bool shuttedDownRD = false;            /**< Flag indicating if the read side has been shutdown. */
    bool shuttedDownWR = false;            /**< Flag indicating if the write side has been shutdown. */
};

} // namespace Sockets
} // namespace Network
} // namespace Mantids30
