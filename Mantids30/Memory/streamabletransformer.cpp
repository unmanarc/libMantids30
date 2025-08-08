#include "streamabletransformer.h"
#include <Mantids30/Memory/streamablenull.h>

void Mantids30::Memory::Streams::StreamableTransformer::transform(
    StreamableObject *in, StreamableObject *out)
{
    this->destObj = out;
    // Launch to the transformer, and the transformer (this) should perform the transformation to out...
    in->streamTo( this );
    // Write the EOF
    this->writeEOF();
    // Reset the dest obj.
    this->destObj = nullptr;
}

std::optional<size_t> Mantids30::Memory::Streams::StreamableTransformer::write(const void *buf, const size_t &count)
{
    size_t r;

    if ( count == 0 ) // EOF
    {
        r = writeTransformerEOF(destObj);

        // // Write the EOF after the transform.
        // destObj->writeEOF();
    }
    else
    {
        r = writeTo(destObj,buf,count);
    }

    if (!destObj->writeStatus.succeed)
        return std::nullopt;

    return r;
}
