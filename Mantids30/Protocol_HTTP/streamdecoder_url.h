#pragma once

#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30 { namespace Memory { namespace Streams { namespace Decoders {

class URL : public Memory::Streams::StreamableObject
{
public:
    URL(std::shared_ptr<Memory::Streams::StreamableObject>  orig);

    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , Status & ) override;
    Status write(const void * buf, const size_t &count, Status &wrStat) override;

    uint64_t getFinalBytesWritten() const;
    void writeEOF(bool) override;

    static std::string decodeURLStr(const std::string & url);

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count, unsigned char *byteDetected);
    Status flushBytes(Status &wrStat);

    unsigned char m_bytes[3];
    uint8_t m_filled;

    uint64_t m_finalBytesWritten;
    std::shared_ptr<Memory::Streams::StreamableObject>  m_orig;
};

}}}}

