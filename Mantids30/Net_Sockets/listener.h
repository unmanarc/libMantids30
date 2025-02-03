#pragma once

#include "callbacks_socket_tcp_server.h"
#include "callbacks_socket_tls_server.h"
#include <memory>

namespace Mantids30 {
namespace Network {
namespace Sockets {

class Listener {
public:
  Listener();

  class Config {
  public:
    Config() = default;

    /**
     * @brief Address to listen on for incoming connections.
     *        Use "*" to listen on all available network interfaces. Default is
     * "*".
     */
    std::string listenAddr = "*";

    /**
     * @brief TCP port number to listen on for incoming connections.
     */
    uint16_t listenPort = 11000;

    /**
     * @brief Path to the Certificate Authority (CA) PEM file for verifying
     * client certificates. If this is empty, client authentication will not be
     * enforced.
     */
    std::string tlsCACertificatePath;

    /**
     * @brief Path to the private key file (PEM format) for the server's TLS
     * certificate.
     */
    std::string tlsPrivateKeyPath;

    /**
     * @brief Path to the server's TLS certificate file (PEM format).
     */
    std::string tlsCertificatePath;

    bool useTLS = true;

    bool useIPv6 = false;

    uint32_t maxConcurrentClients = 16;
    uint32_t maxConnectionsPerIP = 4096;
    // TODO: maxWaitMSTime,
  };

  /**
   * @brief startListening Start a background process listening... every client
   * will be threaded in a new thread... Every client will have a random ID
   * (Session ID)
   * @param parameters server parameters (port/listenaddr/keys/callbacks)
   * @return true if listening, false otherwise.
   */
  bool startListeningInBackground(const Config &parameters);

  // Callbacks from thread:
  virtual int handleClientConnection(
      std::shared_ptr<Sockets::Socket_Stream_Base> stream) = 0;

  // Parameters:
  Config parameters;

  /**
   * @brief TLS-specific server-side callback configuration.
   *        Contains callbacks to handle TLS events (e.g., certificate
   * validation, errors).
   */
  Mantids30::Network::Sockets::Callbacks_Socket_TLS_Server tlsCallbacks;

  /**
   * @brief TCP-specific server-side callback configuration.
   *        Contains callbacks to handle TCP events (e.g., connection
   * establishment, disconnection).
   */
  Mantids30::Network::Sockets::Callbacks_Socket_TCP_Server tcpCallbacks;

  /**
   * @brief User-defined context passed to all configured callbacks.
   *        This allows maintaining application-specific state during callback
   * execution.
   */
  void *listenerContext;

private:
  static bool
  incomingConnection(void *,
                     std::shared_ptr<Sockets::Socket_Stream_Base> bsocket);
};

} // namespace Sockets
} // namespace Network
} // namespace Mantids30
