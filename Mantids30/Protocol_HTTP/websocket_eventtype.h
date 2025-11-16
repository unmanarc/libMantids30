#pragma once

namespace Mantids30::Network::Protocols::WebSocket {

/**
     * @enum WebSocket::EventType
     *
     * @brief Enumeration for different WebSocket event types.
     */
enum EventType
{
    SESSION_START = 0,
    MESSAGE_RECEIVED = 1,
    SESSION_END = 2
};

}
