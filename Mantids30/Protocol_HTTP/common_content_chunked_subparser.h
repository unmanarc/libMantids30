#pragma once

#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30::Network::Protocols::HTTP {

class ContentChunkedTransformer : public Memory::Streams::StreamableObject
{
public:
    ContentChunkedTransformer(Memory::Streams::StreamableObject * upStreamOut);
    ~ContentChunkedTransformer( );

    std::optional<size_t> write(const void *buf, const size_t &count) override;

private:
    size_t m_pos = 0;
    Memory::Streams::StreamableObject * upStreamOut = nullptr;
};

}

