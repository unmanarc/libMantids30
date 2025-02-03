#pragma once

#include <Mantids30/Memory/streamableobject.h>
#include <memory>

namespace Mantids30 { namespace Memory { namespace Streams { namespace Encoders {

class URL : public Memory::Streams::StreamableObject
{
public:
    URL(std::shared_ptr<Memory::Streams::StreamableObject> orig);
    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , Streams::StreamableObject::Status & ) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Streams::StreamableObject::Status &wrStat) override;
    uint64_t getFinalBytesWritten() const;
    static std::string encodeURLStr(const std::string &url);

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count);
    inline bool shouldEncodeThisByte(const unsigned char & byte) const;

    uint64_t m_finalBytesWritten;
    std::shared_ptr<Memory::Streams::StreamableObject>  m_orig;
};


}}}}

