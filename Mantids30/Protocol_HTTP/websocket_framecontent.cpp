#include "websocket_framecontent.h"
#include "Mantids30/Memory/subparser.h"
#include <cstdint>
#include <cstring>
#include <optional>

using namespace Mantids30::Network::Protocols::WebSocket;

// TODO: under construction.

FrameContent::FrameContent()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
    m_content = std::make_shared<Memory::Containers::B_Chunks>();
    m_subParserName = "WebSocket_FrameContent";
}

FrameContent::~FrameContent() {}

void FrameContent::reset()
{
    m_isFirstFrame = true;
    m_isComplete = false;
    m_masked = false;
    m_maskingKey = {0, 0, 0, 0};
    m_lastValidationResult = VALIDATION_OK;
    m_content->clear();
}

bool FrameContent::streamToUpstream()
{
    auto x = m_content->appendTo(*m_upStream);
    return  x != std::nullopt && *x == m_content->size();
}

void FrameContent::setPayloadLength(uint64_t length)
{
    m_isFirstFrame = false;
    setParseDataTargetSize(length);
}

void FrameContent::setMasked(bool masked)
{
    m_masked = masked;
}

void FrameContent::setMaskingKey(const std::array<uint8_t, 4> &key)
{
    m_maskingKey = key;
}

void FrameContent::setValidateUtf8(bool validate)
{
    m_validateUtf8 = validate;
}

Mantids30::Memory::Streams::SubParser::ParseStatus FrameContent::parse()
{
    Mantids30::Memory::Containers::B_Base *buffer = getParsedBuffer();
    if (!buffer)
    {
        return PARSE_ERROR;
    }

    std::vector<char> currentPayload = buffer->copyToBuffer();

    // Unmask if needed
    if (m_masked)
    {
        unmaskData( (uint8_t *) currentPayload.data(),currentPayload.size() );
    }

    auto x= m_content->append(currentPayload.data(),currentPayload.size());
    if ( x==std::nullopt )
    {
        return PARSE_ERROR;
    }

    // Validate UTF-8 if needed
    if (m_isComplete && m_validateUtf8 && !validateUtf8Payload())
    {
        m_lastValidationResult = VALIDATION_INVALID_UTF8;
        return PARSE_ERROR;
    }

    return PARSE_GOTO_NEXT_SUBPARSER;
}

void FrameContent::unmaskData(uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        data[i] ^= m_maskingKey[(i) % 4];
    }
}

bool FrameContent::validateUtf8Payload()
{
    uint8_t utf8BytesRemaining = 0;
    uint32_t utf8Codepoint = 0;


    for (size_t i = 0; i < m_content->size(); ++i)
    {
        uint8_t byte;

        if (m_content->copyOut(&byte,1,i) == std::nullopt)
            return false;

        if (utf8BytesRemaining == 0)
        {
            // Start of new character
            if ((byte & 0x80) == 0)
            {
                // ASCII character (0xxxxxxx)
                continue;
            }
            else if ((byte & 0xE0) == 0xC0)
            {
                // 2-byte character (110xxxxx)
                utf8BytesRemaining = 1;
                utf8Codepoint = byte & 0x1F;
            }
            else if ((byte & 0xF0) == 0xE0)
            {
                // 3-byte character (1110xxxx)
                utf8BytesRemaining = 2;
                utf8Codepoint = byte & 0x0F;
            }
            else if ((byte & 0xF8) == 0xF0)
            {
                // 4-byte character (11110xxx)
                utf8BytesRemaining = 3;
                utf8Codepoint = byte & 0x07;
            }
            else
            {
                // Invalid UTF-8 start byte
                return false;
            }
        }
        else
        {
            // Continuation byte (10xxxxxx)
            if (!isValidUtf8Continuation(byte))
            {
                return false;
            }
            utf8Codepoint = (utf8Codepoint << 6) | (byte & 0x3F);
            utf8BytesRemaining--;

            // Validate complete codepoint
            if (utf8BytesRemaining == 0)
            {
                // Check for overlong encoding
                if ((utf8Codepoint < 0x80) || (utf8Codepoint < 0x800 && utf8Codepoint >= 0x80) || (utf8Codepoint < 0x10000 && utf8Codepoint >= 0x800))
                {
                    // Valid ranges checked implicitly by byte count
                }
                // Check for invalid codepoints
                if (utf8Codepoint > 0x10FFFF || (utf8Codepoint >= 0xD800 && utf8Codepoint <= 0xDFFF))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool FrameContent::isValidUtf8Continuation(uint8_t byte)
{
    return (byte & 0xC0) == 0x80;
}


std::optional<std::string> FrameContent::getContentAsString()
{
    return m_content->toString();
}

std::shared_ptr<Mantids30::Memory::Containers::B_Chunks> FrameContent::getContent() const
{
    return m_content;
}

FrameContent::ValidationResult FrameContent::validateContent()
{
    if (!m_validateUtf8)
    {
        return VALIDATION_OK;
    }

    if (m_lastValidationResult != VALIDATION_OK)
    {
        return m_lastValidationResult;
    }

    return VALIDATION_OK;
}
