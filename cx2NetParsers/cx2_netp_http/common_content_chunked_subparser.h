#ifndef HTTP_HLP_CHUNKED_RETRIEVER_H
#define HTTP_HLP_CHUNKED_RETRIEVER_H

#include <cx2_mem_vars/streamable.h>

namespace CX2 { namespace Network { namespace HTTP { namespace Common {

class Content_Chunked_SubParser : public Memory::Streams::Streamable
{
public:
    Content_Chunked_SubParser( Memory::Streams::Streamable * dst );
    ~Content_Chunked_SubParser( );

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrsStat) override;
    Memory::Streams::Status write(const void * buf, const size_t &count, Memory::Streams::Status & wrStatUpd) override;

    bool endBuffer();

private:
    Memory::Streams::Streamable * dst;
    uint64_t pos;
};

}}}}

#endif // HTTP_HLP_CHUNKED_RETRIEVER_H
