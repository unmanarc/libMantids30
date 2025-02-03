#pragma once

#include <Mantids30/Memory/streamableobject.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class Content_Chunked_SubParser : public Memory::Streams::StreamableObject
{
public:
    Content_Chunked_SubParser( std::shared_ptr<Memory::Streams::StreamableObject> dst );
    ~Content_Chunked_SubParser( );

    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>  out, Memory::Streams::StreamableObject::Status & wrsStat) override;
    Memory::Streams::StreamableObject::Status write(const void * buf, const size_t &count, Memory::Streams::StreamableObject::Status & wrStatUpd) override;

    bool endBuffer();

private:
    std::shared_ptr<Memory::Streams::StreamableObject> m_dst;
    uint64_t m_pos;
};

}}}}}

