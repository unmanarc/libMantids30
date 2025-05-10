#pragma once

#include "socket_stream.h"

namespace Mantids30 {
namespace Network {
namespace Sockets {
namespace Acceptors {

typedef bool (*_callbackConnectionRB)(void *, std::shared_ptr<Sockets::Socket_Stream>);
typedef void (*_callbackConnectionRV)(void *, std::shared_ptr<Sockets::Socket_Stream>);
typedef void (*_callbackConnectionLimit)(void *, std::shared_ptr<Sockets::Socket_Stream>);

class StreamAcceptorThreadCallbacks
{
public:
    _callbackConnectionRB onClientConnected = nullptr;
    _callbackConnectionRB onProtocolInitializationFailure = nullptr;
    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
};

class ThreadPoolCallbacks
{
public:
    void setAllContexts(void *context) { contextOnConnect = contextOnInitFail = contextOnTimedOut = context; }

    _callbackConnectionRB onClientConnected = nullptr;
    _callbackConnectionRB onProtocolInitializationFailure = nullptr;
    _callbackConnectionRV onClientAcceptTimeoutOccurred = nullptr;
    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
    void *contextOnTimedOut = nullptr;
};

class MultiThreadCallbacks
{
public:
    void setAllContexts(void *context) { contextOnConnect = contextOnInitFail = contextOnTimedOut = contextonClientConnectionLimitPerIPReached = context; }

    // Callbacks:
    _callbackConnectionRB onClientConnected = nullptr;
    _callbackConnectionRB onProtocolInitializationFailure = nullptr;
    _callbackConnectionRV onClientAcceptTimeoutOccurred = nullptr;
    _callbackConnectionLimit onClientConnectionLimitPerIPReached = nullptr;

    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
    void *contextOnTimedOut = nullptr;
    void *contextonClientConnectionLimitPerIPReached = nullptr;
};

} // namespace Acceptors
} // namespace Sockets
} // namespace Network
} // namespace Mantids30
