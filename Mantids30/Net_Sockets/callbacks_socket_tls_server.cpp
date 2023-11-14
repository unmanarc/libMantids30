#include "callbacks_socket_tls_server.h"

using namespace Mantids30::Network::Sockets;

Callbacks_Socket_TLS_Server::Callbacks_Socket_TLS_Server(void * obj) : Callbacks_Socket_TLS(obj)
{
    this->onTLSListeningSuccess=nullptr;
    this->onTLSListeningFailed=nullptr;
    this->onTLSClientDisconnected=nullptr;
    this->onTLSClientConnected=nullptr;
    this->onTLSClientAuthenticationError=nullptr;
    this->onTLSClientAcceptTimeoutOccurred=nullptr;
    this->onTLSClientConnectionLimitPerIPReached=nullptr;
}

