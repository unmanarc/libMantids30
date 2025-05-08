#include "streamabletransformer.h"


Mantids30::Memory::Streams::WriteStatus Mantids30::Memory::Streams::StreamableTransformer::transform(
    StreamableObject *in, StreamableObject *out)
{
    Mantids30::Memory::Streams::WriteStatus status;
    this->destObj = out;
    // Launch to the transformer, and the transformer (this) should perform the transformation to out...
    in->streamTo( this, status );
    this->destObj = nullptr;
    return status;
}
