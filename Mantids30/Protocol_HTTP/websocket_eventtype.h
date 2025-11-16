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
    RECEIVED_MESSAGE_BINARY = 1,
    RECEIVED_MESSAGE_TEXT = 2,
    SESSION_END = 3
};

}
