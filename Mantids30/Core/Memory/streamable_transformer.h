#pragma once

#include <cstdlib>
#include <optional>

#include "streamable_object.h"

namespace Mantids30::Core::Memory::Streams {

/**
 * StreamableTransformer base class
 * This is a base class for streamable objects that can be retrieved or parsed trough read/write functions.
 */
class StreamableTransformer : private StreamableObject
{
public:
    StreamableTransformer() = default;
    ~StreamableTransformer() override = default;
    void transform(Mantids30::Core::Memory::Streams::StreamableObject *in, Mantids30::Core::Memory::Streams::StreamableObject *out);

protected:
    virtual size_t writeTo(Mantids30::Core::Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count) = 0;
    virtual size_t writeTransformerEOF(Mantids30::Core::Memory::Streams::StreamableObject *dst) { return 0; }

private:
    std::optional<size_t> write(const void *buf, const size_t &count) override;

    Mantids30::Core::Memory::Streams::StreamableObject *destObj = nullptr;
};

} // namespace Mantids30::Core::Memory::Streams
