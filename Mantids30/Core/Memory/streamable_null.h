#pragma once

#include "streamable_object.h"
#include <unistd.h>

namespace Mantids30::Core::Memory::Streams {

class StreamableNull : public Mantids30::Core::Memory::Streams::StreamableObject
{
public:
    StreamableNull() = default;
    std::optional<size_t> writeImpl(const void *buf, const size_t &count) override;
    size_t size() override { return bytes; }

private:
    size_t bytes = 0;
};

} // namespace Mantids30::Core::Memory::Streams
