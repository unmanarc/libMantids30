#ifndef STREAMABLENULL_H
#define STREAMABLENULL_H

#include "streamableobject.h"
#include <unistd.h>

namespace Mantids { namespace Memory { namespace Streams {

class StreamableNull : public Memory::Streams::StreamableObject
{
public:
    StreamableNull();
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    virtual bool streamTo(Memory::Streams::StreamableObject * out, Status & wrStatUpd) override;
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd) override;

private:
};

}}}

#endif // STREAMABLENULL_H
