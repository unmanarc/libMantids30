#ifndef STREAMDECURL_H
#define STREAMDECURL_H

#include <mdz_mem_vars/streamableobject.h>

namespace Mantids { namespace Memory { namespace Streams { namespace Decoders {

class URL : public Memory::Streams::StreamableObject
{
public:
    URL(Memory::Streams::StreamableObject * orig);

    bool streamTo(Memory::Streams::StreamableObject *, Status & ) override;
    Status write(const void * buf, const size_t &count, Status &wrStat) override;

    uint64_t getFinalBytesWritten() const;
    void writeEOF(bool) override;

    static std::string decodeURLStr(const std::string & url);

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count, unsigned char *byteDetected);
    Status flushBytes(Status &wrStat);

    inline unsigned char hex2uchar();
    inline bool isHexByte(unsigned char b);
    inline unsigned char get16Value(unsigned char byte);
    unsigned char bytes[3];
    uint8_t filled;

    uint64_t finalBytesWritten;
    Memory::Streams::StreamableObject * orig;
};

}}}}

#endif // STREAMDECURL_H
