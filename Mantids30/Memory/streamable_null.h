#pragma once

#include "streamable_object.h"
#include <unistd.h>

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableNull : public Memory::Streams::StreamableObject
{
public:
    StreamableNull() = default;
    virtual std::optional<size_t> write(const void *buf, const size_t &count) override;
    size_t size() override { return bytes; }

private:
    size_t bytes = 0;
};

}}}

