#pragma once

#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP {

class ContentChunkedTransformer : public Memory::Streams::StreamableObject
{
public:
    ContentChunkedTransformer(Memory::Streams::StreamableObject * upStreamOut);
    ~ContentChunkedTransformer( );

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus & wrsStat) override;
    Memory::Streams::WriteStatus write(const void * buf, const size_t &count, Memory::Streams::WriteStatus & wrStatUpd) override;

    bool endBuffer();

private:
    uint64_t m_pos = 0;
    Memory::Streams::StreamableObject * upStreamOut = nullptr;
};

}}}}

