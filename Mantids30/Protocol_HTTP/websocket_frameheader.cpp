#include "websocket_frameheader.h"
#include <cstring>
#include <netinet/in.h>
#include <optional>

using namespace Mantids30::Network::Protocols::WebSocket;

FrameHeader::FrameHeader()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
    setParseDataTargetSize(2); // Start with first 2 bytes
    m_subParserName = "WebSocket_FrameHeader";
}

FrameHeader::~FrameHeader() {}

void FrameHeader::reset()
{
    m_fin = false;
    m_rsv1 = false;
    m_rsv2 = false;
    m_rsv3 = false;
    m_opcode = OPCODE_CONTINUATION;
    m_masked = false;
    m_payloadLength = 0;
    m_maskingKey = {0, 0, 0, 0};
    m_parseState = STATE_FIRST_2BYTES;
    m_extendedLengthBytes = 0;
    m_extendedLengthBytesRead = 0;
    m_maskingKeyBytesRead = 0;
    m_lastError = ERROR_NONE;
    setParseDataTargetSize(2);
}

bool FrameHeader::isControlFrame() const
{
    return (m_opcode & 0x08) != 0;
}

bool FrameHeader::isDataFrame() const
{
    return !isControlFrame();
}

bool FrameHeader::isValidOpCode() const
{
    return m_opcode <= OPCODE_BINARY || (m_opcode >= OPCODE_CLOSE && m_opcode <= OPCODE_PONG);
}

bool FrameHeader::streamToUpstream()
{
    // First byte: FIN + RSV1-3 + Opcode
    uint8_t firstByte = 0;
    if (m_fin)
        firstByte |= 0x80;
    if (m_rsv1)
        firstByte |= 0x40;
    if (m_rsv2)
        firstByte |= 0x20;
    if (m_rsv3)
        firstByte |= 0x10;
    firstByte |= (m_opcode & 0x0F);

    // Second byte: MASK + Payload length
    uint8_t secondByte = 0;
    if (m_masked)
        secondByte |= 0x80;

    char bytes[2];
    bytes[0] = static_cast<char>(firstByte);

    // Handle payload length encoding
    uint8_t payloadLenField;
    if (m_payloadLength <= 125)
    {
        payloadLenField = static_cast<uint8_t>(m_payloadLength);
    }
    else if (m_payloadLength <= 65535)
    {
        payloadLenField = 126;
    }
    else
    {
        payloadLenField = 127;
    }

    secondByte |= (payloadLenField & 0x7F);
    bytes[1] = static_cast<char>(secondByte);

    // Send first 2 bytes
    if (!m_upStream->writeFullStream(bytes, 2))
    {
        return false;
    }

    // Send extended payload length if needed
    if (payloadLenField == 126)
    {
        // 16-bit extended length (network byte order)
        uint16_t extendedLen = htons(static_cast<uint16_t>(m_payloadLength));
        if (!m_upStream->writeFullStream(reinterpret_cast<char *>(&extendedLen), 2))
        {
            return false;
        }
    }
    else if (payloadLenField == 127)
    {
        // 64-bit extended length (network byte order)
        uint64_t extendedLen = htobe64(m_payloadLength);
        if (!m_upStream->writeFullStream(reinterpret_cast<char *>(&extendedLen), 8))
        {
            return false;
        }
    }

    // Send masking key if masked
    if (m_masked)
    {
        if (!m_upStream->writeFullStream(reinterpret_cast<const char *>(m_maskingKey.data()), 4))
        {
            return false;
        }
    }

    return true;
}

void FrameHeader::prepareCloseFrame(uint64_t payloadLength)
{
    reset();
    m_fin = true;
    m_opcode = OPCODE_CLOSE;
    m_payloadLength = payloadLength;
    // Control frames must be <= 125 bytes
    if (payloadLength > 125)
    {
        m_payloadLength = 125;
    }
}

void FrameHeader::preparePongFrame(uint64_t payloadLength)
{
    reset();
    m_fin = true;
    m_opcode = OPCODE_PONG;
    m_payloadLength = payloadLength;
    // Control frames must be <= 125 bytes
    if (payloadLength > 125)
    {
        m_payloadLength = 125;
    }
}

void FrameHeader::preparePingFrame(uint64_t payloadLength)
{
    reset();
    m_fin = true;
    m_opcode = OPCODE_PING;
    m_payloadLength = payloadLength;
    // Control frames must be <= 125 bytes
    if (payloadLength > 125)
    {
        m_payloadLength = 125;
    }
}

void FrameHeader::prepareHeader(bool fin, OpCode opcode, uint64_t payloadLength, bool masked, const std::array<uint8_t, 4> &maskingKey)
{
    m_fin = fin;
    m_opcode = opcode;
    m_payloadLength = payloadLength;
    m_masked = masked;

    if (masked)
    {
        m_maskingKey = maskingKey;
    }
    else
    {
        m_maskingKey = {0, 0, 0, 0};
    }

    // You might also reset any parser state here since this is for outbound use
    m_parseState = STATE_COMPLETE;  // Indicate that the header is ready to be written
}

Mantids30::Memory::Streams::SubParser::ParseStatus FrameHeader::parse()
{
    switch (m_parseState)
    {
    case STATE_FIRST_2BYTES:
    {
        uint8_t data[2] = {0, 0};

        std::optional<size_t> x = getParsedBuffer()->copyOut(data, 2, 0);
        if (x == std::nullopt || *x != 2)
        {
            // Error, parsed buffer is not delivered as expected.
            return PARSE_ERROR;
        }
        if (!parseFirstByte(data[0]) || !parseSecondByte(data[1]))
        {
            return PARSE_ERROR;
        }
        // Determine next state based on payload length
        if (m_payloadLength == 126)
        {
            m_parseState = STATE_EXTENDED_LENGTH_16;
            m_extendedLengthBytes = 2;
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
            setParseDataTargetSize(2);
            return PARSE_GET_MORE_DATA;
        }
        else if (m_payloadLength == 127)
        {
            m_parseState = STATE_EXTENDED_LENGTH_64;
            m_extendedLengthBytes = 8;
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
            setParseDataTargetSize(8);
            return PARSE_GET_MORE_DATA;
        }
        else if (m_masked)
        {
            m_parseState = STATE_MASKING_KEY;
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
            setParseDataTargetSize(4);
            return PARSE_GET_MORE_DATA;
        }
        else
        {
            m_parseState = STATE_COMPLETE;
            return PARSE_GOTO_NEXT_SUBPARSER;
        }
    }
    break;
    case STATE_EXTENDED_LENGTH_16:
    case STATE_EXTENDED_LENGTH_64:
        if (!parseExtendedLength())
        {
            return PARSE_ERROR;
        }
        if (m_masked)
        {
            m_parseState = STATE_MASKING_KEY;
            return PARSE_GET_MORE_DATA;
        }
        else
        {
            m_parseState = STATE_COMPLETE;
            return PARSE_GOTO_NEXT_SUBPARSER;
        }
        break;

    case STATE_MASKING_KEY:
        if (!parseMaskingKey())
        {
            return PARSE_ERROR;
        }
        m_parseState = STATE_COMPLETE;
        return PARSE_GOTO_NEXT_SUBPARSER;

    case STATE_COMPLETE:
        return PARSE_GOTO_NEXT_SUBPARSER;
    }

    return PARSE_ERROR;
}

bool FrameHeader::parseFirstByte(uint8_t byte)
{
    m_fin = (byte & 0x80) != 0;
    m_rsv1 = (byte & 0x40) != 0;
    m_rsv2 = (byte & 0x20) != 0;
    m_rsv3 = (byte & 0x10) != 0;
    m_opcode = static_cast<OpCode>(byte & 0x0F);

    // Validate RSV bits (should be 0 unless extensions are negotiated)
    if (m_rsv1 || m_rsv2 || m_rsv3)
    {
        m_lastError = ERROR_INVALID_RSV_BITS;
        return false;
    }

    // Validate opcode
    if (!isValidOpCode())
    {
        m_lastError = ERROR_INVALID_OPCODE;
        return false;
    }

    // Control frames must not be fragmented
    if (isControlFrame() && !m_fin)
    {
        m_lastError = ERROR_FRAGMENTED_CONTROL_FRAME;
        return false;
    }

    return true;
}

bool FrameHeader::parseSecondByte(uint8_t byte)
{
    m_masked = (byte & 0x80) != 0;
    m_payloadLength = byte & 0x7F;

    // Validate masking requirement
    if (m_requireMasking && !m_masked)
    {
        m_lastError = ERROR_INVALID_OPCODE;
        return false;
    }

    // Control frames must have payload length <= 125
    if (isControlFrame() && m_payloadLength > 125)
    {
        m_lastError = ERROR_CONTROL_FRAME_TOO_LARGE;
        return false;
    }

    return true;
}

bool FrameHeader::parseExtendedLength()
{
    if (m_extendedLengthBytes == 2)
    {
        uint8_t data[2];
        auto x = getParsedBuffer()->copyOut(data, 2, 0);
        if (x == std::nullopt || *x != 2)
        {
            return false;
        }
        // 16-bit extended length
        m_payloadLength = (static_cast<uint64_t>(data[2]) << 8) | static_cast<uint64_t>(data[3]);
    }
    else if (m_extendedLengthBytes == 8)
    {
        uint8_t data[8];
        auto x = getParsedBuffer()->copyOut(data, 8, 0);
        if (x == std::nullopt || *x != 8)
        {
            return false;
        }

        // 64-bit extended length
        m_payloadLength = 0;
        for (int i = 0; i < 8; ++i)
        {
            m_payloadLength = (m_payloadLength << 8) | data[2 + i];
        }
    }
    else
    {
        throw std::runtime_error("WebSocket: Invalid extended length bytes.");
    }

    // Validate payload size
    if (m_payloadLength > m_maxPayloadSize)
    {
        m_lastError = ERROR_PAYLOAD_TOO_LARGE;
        return false;
    }

    return true;
}

bool FrameHeader::parseMaskingKey()
{
    uint8_t data[4];
    auto x = getParsedBuffer()->copyOut(data, 4, 0);
    if (x == std::nullopt || *x != 4)
    {
        return false;
    }
    std::memcpy(m_maskingKey.data(), data, 4);

    return true;
}
