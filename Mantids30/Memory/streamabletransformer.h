#pragma once

#include <stdlib.h>

#include "streamableobject.h"

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

    bool streamTo(Memory::Streams::StreamableObject *, Streams::WriteStatus & ) override { return false; }

    Streams::WriteStatus transform( Memory::Streams::StreamableObject * in, Memory::Streams::StreamableObject * out );

protected:
    bool setFailedWriteState(const uint16_t &value = 1)
    {
        return StreamableObject::setFailedWriteState(value);
    }

    virtual Memory::Streams::WriteStatus writeTo(Memory::Streams::StreamableObject * dst, const void * buf, const size_t &count, Streams::WriteStatus &wrStat) = 0;

    virtual void writeTransformerEOF(Memory::Streams::StreamableObject *dst, bool) {}


private:
    /**
     * @brief writeEOF proccess the end of the stream (should be implemented on streamTo)
     */
    void writeEOF(bool x) override
    {
        writeTransformerEOF(destObj,x);
    }

    Memory::Streams::WriteStatus write(const void * buf, const size_t &count, Streams::WriteStatus &wrStat) override
    {
        return writeTo(destObj,buf,count,wrStat);
    }

    Memory::Streams::StreamableObject * destObj = nullptr;

};

}}}



