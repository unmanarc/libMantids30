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

    void transform( Memory::Streams::StreamableObject * in, Memory::Streams::StreamableObject * out );


protected:

    virtual size_t writeTo(Memory::Streams::StreamableObject * dst, const void * buf, const size_t &count) = 0;
    virtual size_t writeTransformerEOF(Memory::Streams::StreamableObject *dst) { return 0; }

private:
    size_t write(const void * buf, const size_t &count) override
    {
        size_t r;

        if ( count == 0 ) // EOF
        {
            r = writeTransformerEOF(destObj);
        }
        else
        {
            r = writeTo(destObj,buf,count);
        }

        return r;

    }

    Memory::Streams::StreamableObject * destObj = nullptr;

};

}}}



