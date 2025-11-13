#pragma once

#include <Mantids30/Memory/subparser.h>
#include <array>
#include <boost/optional.hpp>
#include <cstdint>

namespace Mantids30::Network::Protocols::WebSocket {

class FrameHeader : public Memory::Streams::SubParser
{
public:
    enum OpCode : uint8_t
    {
        OPCODE_CONTINUATION = 0x0,
        OPCODE_TEXT = 0x1,
        OPCODE_BINARY = 0x2,
        OPCODE_CLOSE = 0x8,
        OPCODE_PING = 0x9,
        OPCODE_PONG = 0xA
    };

    enum ParseState
    {
        STATE_FIRST_2BYTES,
        STATE_EXTENDED_LENGTH_16,
        STATE_EXTENDED_LENGTH_64,
        STATE_MASKING_KEY,
        STATE_COMPLETE
    };

    enum ErrorCode
    {
        ERROR_NONE = 0,
        ERROR_INVALID_OPCODE,
        ERROR_INVALID_RSV_BITS,
        ERROR_CONTROL_FRAME_TOO_LARGE,
        ERROR_FRAGMENTED_CONTROL_FRAME,
        ERROR_INVALID_UTF8,
        ERROR_PAYLOAD_TOO_LARGE
    };

    FrameHeader();
    virtual ~FrameHeader();

    // Reset the header for parsing a new frame
    void reset();

    // Getters for frame properties
    bool isFinalFragment() const { return m_fin; }
    bool isRsv1Set() const { return m_rsv1; }
    bool isRsv2Set() const { return m_rsv2; }
    bool isRsv3Set() const { return m_rsv3; }
    OpCode getOpCode() const { return m_opcode; }
    bool isMasked() const { return m_masked; }
    uint64_t getPayloadLength() const { return m_payloadLength; }
    const std::array<uint8_t, 4> &getMaskingKey() const { return m_maskingKey; }

    // Validation helpers
    bool isControlFrame() const;
    bool isDataFrame() const;
    bool isValidOpCode() const;

    ErrorCode getLastError() const { return m_lastError; }

    // Configuration
    void setMaxPayloadSize(uint64_t maxSize) { m_maxPayloadSize = maxSize; }
    void setRequireMasking(bool require) { m_requireMasking = require; }

    bool streamToUpstream( ) override;


    // Close connection response
    void prepareCloseFrame(uint64_t payloadLength);

    // Pong response (for handling ping)
    void preparePongFrame(uint64_t payloadLength);

    // Ping initiation
    void preparePingFrame(uint64_t payloadLength);

    void prepareHeader(bool fin, OpCode opcode, uint64_t payloadLength, bool masked, const std::array<uint8_t, 4>& maskingKey = {0,0,0,0});

protected:
    virtual ParseStatus parse() override;
private:
    bool parseFirstByte(uint8_t byte);
    bool parseSecondByte(uint8_t byte);
    bool parseExtendedLength();
    bool parseMaskingKey();

    // Header fields
    bool m_fin = false;
    bool m_rsv1 = false;
    bool m_rsv2 = false;
    bool m_rsv3 = false;
    OpCode m_opcode = OPCODE_CONTINUATION;
    bool m_masked = false;
    uint64_t m_payloadLength = 0;
    std::array<uint8_t, 4> m_maskingKey = {0, 0, 0, 0};

    // Parsing state
    ParseState m_parseState = STATE_FIRST_2BYTES;
    uint8_t m_extendedLengthBytes = 0;
    uint8_t m_extendedLengthBytesRead = 0;
    uint8_t m_maskingKeyBytesRead = 0;

    // Configuration
    uint64_t m_maxPayloadSize = 256 * 1024; // 256kb default
    bool m_requireMasking = true;           // true for server, false for client

    // Error tracking
    ErrorCode m_lastError = ERROR_NONE;
};

} // namespace Mantids30::Network::Protocols::WebSocket
