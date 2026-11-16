#pragma once

#include <Mantids30/Core/Memory/streamable_transformer.h>
#include <cstdint>

namespace Mantids30::Protocol::HTTP::Decoders {

class URL : public Mantids30::Core::Memory::Streams::StreamableTransformer
{
public:
    URL() = default;
    static std::string decodeURLStr(const std::string &url);

protected:
    size_t writeTo(Mantids30::Core::Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count) override;
    size_t writeTransformerEOF(Mantids30::Core::Memory::Streams::StreamableObject *dst) override;

private:
    size_t getPlainBytesSize(const unsigned char *buf, size_t count, unsigned char *byteDetected);
    bool flushBytesAndResetFilledCounter(Mantids30::Core::Memory::Streams::StreamableObject *dst);

    unsigned char m_bytes[3]{0,0,0};
    uint8_t m_filled = 0;
};

} // namespace Mantids30::Protocol::HTTP::Decoders
