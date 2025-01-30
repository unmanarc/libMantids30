#pragma once

#include "streamableobject.h"

namespace Mantids30 { namespace Memory { namespace Containers {

class NullContainer : public Streams::StreamableObject
{
public:
    NullContainer();

    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>  out, Streams::StreamableObject::Status & wrsStat) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Streams::StreamableObject::Status & wrStatUpd) override;

    uint64_t size() const override { return bytes; }

private:
    uint64_t bytes;
};

}}}

