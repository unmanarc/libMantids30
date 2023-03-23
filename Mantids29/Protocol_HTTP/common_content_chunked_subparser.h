#ifndef HTTP_HLP_CHUNKED_RETRIEVER_H
#define HTTP_HLP_CHUNKED_RETRIEVER_H

#include <Mantids29/Memory/streamableobject.h>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class Content_Chunked_SubParser : public Memory::Streams::StreamableObject
{
public:
    Content_Chunked_SubParser( Memory::Streams::StreamableObject * dst );
    ~Content_Chunked_SubParser( );

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::StreamableObject::Status & wrsStat) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Memory::Streams::StreamableObject::Status & wrStatUpd) override;

    bool endBuffer();

private:
    Memory::Streams::StreamableObject * dst;
    uint64_t pos;
};

}}}}}

#endif // HTTP_HLP_CHUNKED_RETRIEVER_H
