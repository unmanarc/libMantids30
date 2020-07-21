#ifndef STREAMDECURL_H
#define STREAMDECURL_H

#include <cx2_mem_streams/streamable.h>

namespace CX2 { namespace Memory { namespace Streams { namespace Decoders {

class URL : public Memory::Streams::Streamable
{
public:
    URL(Memory::Streams::Streamable * orig);

    bool streamTo(Memory::Streams::Streamable *, Status & ) override;
    Status write(const void * buf, const size_t &count, Status &wrStat) override;

    uint64_t getFinalBytesWritten() const;
    void writeEOF(bool) override;

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count);
    Status flushBytes(Status &wrStat);

    inline unsigned char hex2uchar();
    inline bool isHexByte(unsigned char b);
    inline unsigned char get16Value(unsigned char byte);
    unsigned char bytes[3];
    uint8_t filled;

    uint64_t finalBytesWritten;
    Memory::Streams::Streamable * orig;
};

}}}}

#endif // STREAMDECURL_H
