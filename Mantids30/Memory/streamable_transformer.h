#pragma once

#include <optional>
#include <stdlib.h>

#include "streamable_object.h"

namespace Mantids30 { namespace Memory { namespace Streams {

/**
 * StreamableTransformer base class
 * This is a base class for streamable objects that can be retrieved or parsed trough read/write functions.
 */
class StreamableTransformer : private StreamableObject
{
public:
    StreamableTransformer() = default;
    virtual ~StreamableTransformer() = default;
    void transform( Memory::Streams::StreamableObject * in, Memory::Streams::StreamableObject * out );


protected:

    virtual size_t writeTo(Memory::Streams::StreamableObject * dst, const void * buf, const size_t &count) = 0;
    virtual size_t writeTransformerEOF(Memory::Streams::StreamableObject *dst) { return 0; }

private:
    std::optional<size_t> write(const void * buf, const size_t &count) override;

    Memory::Streams::StreamableObject * destObj = nullptr;

};

}}}



