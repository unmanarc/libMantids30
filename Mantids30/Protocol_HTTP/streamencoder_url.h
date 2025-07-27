#pragma once

#include "Mantids30/Memory/streamabletransformer.h"
#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30 { namespace Memory { namespace Streams { namespace Encoders {

class URL : public Memory::Streams::StreamableTransformer
{
public:
    URL() = default;
    static std::string encodeURLStr(const std::string &url);

protected:
    size_t writeTo(Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count) override;

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count);
    inline bool shouldEncodeThisByte(const unsigned char & byte) const;

    //size_t m_finalBytesWritten = 0;
};


}}}}
