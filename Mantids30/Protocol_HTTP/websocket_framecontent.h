#pragma once

#include <Mantids30/Memory/subparser.h>
#include <array>
#include <boost/optional.hpp>
#include <cstdint>
#include <memory>

namespace Mantids30::Network::Protocols::WebSocket {

class FrameContent : public Memory::Streams::SubParser
{
public:
    enum ValidationResult
    {
        VALIDATION_OK,
        VALIDATION_INVALID_UTF8,
        VALIDATION_INCOMPLETE_UTF8,
        VALIDATION_ERROR
    };

    FrameContent();
    virtual ~FrameContent();

    // Configuration
    void setPayloadLength(uint64_t length);
    void setMasked(bool masked);
    void setMaskingKey(const std::array<uint8_t, 4> &key);
    void setValidateUtf8(bool validate);

    // Get content as string
    std::optional<std::string> getContentAsString();

    // Get content as a binary container.
    std::shared_ptr<Memory::Containers::B_Chunks> getContent() const;

    // Validation
    ValidationResult validateContent();
    bool isComplete() const { return m_isComplete; }
    void setComplete(bool complete) { m_isComplete = complete; }
    bool isFirstFrame() const { return m_isFirstFrame; }
    void setMaxContentSize(uint64_t maxSize) { m_maxContentSize = maxSize; }

    // Reset for new frame
    void reset();

    bool streamToUpstream( ) override;

protected:
    virtual ParseStatus parse() override;

private:
    void unmaskData(uint8_t *data, size_t length);
    bool validateUtf8Payload();
    bool isValidUtf8Continuation(uint8_t byte);

    // Configuration
    bool m_masked = false;
    std::array<uint8_t, 4> m_maskingKey = {0, 0, 0, 0};
    bool m_validateUtf8 = false;

    // Content storage
    std::shared_ptr<Memory::Containers::B_Chunks> m_content;

    bool m_isComplete = false;
    bool m_isFirstFrame = false;

    // UTF-8 validation state
    ValidationResult m_lastValidationResult = VALIDATION_OK;

    uint64_t m_maxContentSize = 512 * 1024; // 512Kb default
};

} // namespace Mantids30::Network::Protocols::WebSocket
