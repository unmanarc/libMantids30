#pragma once

#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class Content_Chunked_SubParser : public Memory::Streams::StreamableObject
{
public:
    Content_Chunked_SubParser(Memory::Streams::StreamableObject * upStreamOut);
    ~Content_Chunked_SubParser( );

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus & wrsStat) override;
    Memory::Streams::WriteStatus write(const void * buf, const size_t &count, Memory::Streams::WriteStatus & wrStatUpd) override;

    bool endBuffer();

private:
    uint64_t m_pos = 0;
    Memory::Streams::StreamableObject * upStreamOut = nullptr;
};

}}}}}

