#pragma once

#include "streamableobject.h"
#include <unistd.h>

namespace Mantids30 { namespace Memory { namespace Streams {

class StreamableNull : public Memory::Streams::StreamableObject
{
public:
    StreamableNull() = default;
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, WriteStatus & wrStatUpd) override;
    virtual WriteStatus write(const void * buf, const size_t &count, WriteStatus & wrStatUpd) override;

private:
};

}}}

