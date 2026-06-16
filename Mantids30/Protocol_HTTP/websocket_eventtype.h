#pragma once

#include <cstdint>
namespace Mantids30::Network::Protocol::WebSocket {

/**
     * @enum WebSocket::EventType
     *
     * @brief Enumeration for different WebSocket event types.
     */
enum class EventType : uint8_t
{
    SESSION_START = 0,
    RECEIVED_MESSAGE_BINARY = 1,
    RECEIVED_MESSAGE_TEXT = 2,
    SESSION_END = 3
};

} // namespace Mantids30::Network::Protocol::WebSocket
