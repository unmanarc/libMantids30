#pragma once

#include "streamableobject.h"

namespace Mantids30 { namespace Memory { namespace Containers {

class NullContainer : public Streams::StreamableObject
{
public:
    NullContainer();

    bool streamTo(Memory::Streams::StreamableObject * out, Streams::WriteStatus & wrsStat) override;
    Memory::Streams::WriteStatus write(const void * buf, const size_t &count, Streams::WriteStatus & wrStatUpd) override;

    uint64_t size() const override { return bytes; }

private:
    uint64_t bytes;
};

}}}

