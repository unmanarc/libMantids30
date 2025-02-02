#pragma once

#include "socket_stream_base.h"

namespace Mantids30 {
namespace Network {
namespace Sockets {
namespace Acceptors {

typedef bool (*_callbackConnectionRB)(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);
typedef void (*_callbackConnectionRV)(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);
typedef void (*_callbackConnectionLimit)(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *);

class SAThreadCallbacks
{
public:
    _callbackConnectionRB onConnect = nullptr;
    _callbackConnectionRB onInitFail = nullptr;
    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
};

class ThreadPoolCallbacks
{
public:
    void setAllContexts(void *context) { contextOnConnect = contextOnInitFail = contextOnTimedOut = context; }

    _callbackConnectionRB onConnect = nullptr;
    _callbackConnectionRB onInitFail = nullptr;
    _callbackConnectionRV onTimedOut = nullptr;
    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
    void *contextOnTimedOut = nullptr;
};

class MultiThreadCallbacks
{
public:
    void setAllContexts(void *context) { contextOnConnect = contextOnInitFail = contextOnTimedOut = contextOnMaxConnectionsPerIP = context; }

    // Callbacks:
    _callbackConnectionRB onConnect = nullptr;
    _callbackConnectionRB onInitFail = nullptr;
    _callbackConnectionRV onTimedOut = nullptr;
    _callbackConnectionLimit onMaxConnectionsPerIP = nullptr;

    void *contextOnConnect = nullptr;
    void *contextOnInitFail = nullptr;
    void *contextOnTimedOut = nullptr;
    void *contextOnMaxConnectionsPerIP = nullptr;
};

} // namespace Acceptors
} // namespace Sockets
} // namespace Network
} // namespace Mantids30
