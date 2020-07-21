#ifndef HTTP_HLP_CHUNKED_RETRIEVER_H
#define HTTP_HLP_CHUNKED_RETRIEVER_H

#include <cx2_mem_streams/streamable.h>

namespace CX2 { namespace Network { namespace Parsers {

class HTTP_HLP_Chunked_Retriever : public Memory::Streams::Streamable
{
public:
    HTTP_HLP_Chunked_Retriever( Memory::Streams::Streamable * dst );
    ~HTTP_HLP_Chunked_Retriever( );

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrsStat) override;
    Memory::Streams::Status write(const void * buf, const size_t &count, Memory::Streams::Status & wrStatUpd) override;


    bool endBuffer();

private:
    Memory::Streams::Streamable * dst;
    uint64_t pos;
};

}}}

#endif // HTTP_HLP_CHUNKED_RETRIEVER_H
