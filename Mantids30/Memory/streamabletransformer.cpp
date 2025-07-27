#include "streamabletransformer.h"
#include <Mantids30/Memory/streamablenull.h>

void Mantids30::Memory::Streams::StreamableTransformer::transform(
    StreamableObject *in, StreamableObject *out)
{
    this->destObj = out;
    // Launch to the transformer, and the transformer (this) should perform the transformation to out...
    in->streamTo( this );
    // Reset the dest obj.
    this->destObj = nullptr;
}
