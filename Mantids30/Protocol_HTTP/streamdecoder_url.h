#pragma once

#include <Mantids30/Memory/streamabletransformer.h>

namespace Mantids30 { namespace Memory { namespace Streams { namespace Decoders {

class URL : public Memory::Streams::StreamableTransformer
{
public:
    URL() = default;

    static std::string decodeURLStr(const std::string & url);

protected:
    size_t writeTo(Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count) override;

    size_t writeTransformerEOF(Memory::Streams::StreamableObject *dst) override;

    //void writeTransformerEOF(Memory::Streams::StreamableObject *dst, bool) override;

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count, unsigned char *byteDetected);
    bool flushBytesAndResetFilledCounter(Memory::Streams::StreamableObject *dst);

    unsigned char m_bytes[3];
    uint8_t m_filled = 0;
    //size_t m_finalBytesWritten = 0;


};

}}}}

