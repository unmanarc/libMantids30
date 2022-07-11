#ifndef STREAMURL_H
#define STREAMURL_H

#include <mdz_mem_vars/streamableobject.h>

namespace Mantids { namespace Memory { namespace Streams { namespace Encoders {

class URL : public Memory::Streams::StreamableObject
{
public:
    URL(Memory::Streams::StreamableObject * orig);
    bool streamTo(Memory::Streams::StreamableObject *, Streams::StreamableObject::Status & ) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Streams::StreamableObject::Status &wrStat) override;
    uint64_t getFinalBytesWritten() const;
    static std::string encodeURLStr(const std::string &url);

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count);
    inline bool shouldEncodeThisByte(const unsigned char & byte) const;

    uint64_t finalBytesWritten;
    Memory::Streams::StreamableObject * orig;
};


}}}}

#endif // STREAMURL_H
