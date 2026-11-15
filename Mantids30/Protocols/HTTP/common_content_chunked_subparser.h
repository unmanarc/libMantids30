#pragma once

#include <Mantids30/Core/Memory/streamable_object.h>

namespace Mantids30::Protocols::HTTP {

class ContentChunkedTransformer : public Mantids30::Core::Memory::Streams::StreamableObject
{
public:
    ContentChunkedTransformer(Mantids30::Core::Memory::Streams::StreamableObject *upStreamOut);
    ~ContentChunkedTransformer() override;

    std::optional<size_t> write(const void *buf, const size_t &count) override;

private:
    size_t m_pos = 0;
    Mantids30::Core::Memory::Streams::StreamableObject *upStreamOut = nullptr;
};

} // namespace Mantids30::Protocols::HTTP
