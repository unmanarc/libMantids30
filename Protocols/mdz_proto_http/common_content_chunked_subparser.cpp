#include "common_content_chunked_subparser.h"
#include <stdio.h>
#include <string.h>

using namespace Mantids::Protocols::HTTP;
using namespace Mantids::Protocols::HTTP::Common;
using namespace Mantids;

Content_Chunked_SubParser::Content_Chunked_SubParser(Memory::Streams::StreamableObject *dst)
{
    this->dst = dst;
    pos = 0;
}

Content_Chunked_SubParser::~Content_Chunked_SubParser()
{
    endBuffer();
}

bool Content_Chunked_SubParser::streamTo(Memory::Streams::StreamableObject *out, Memory::Streams::StreamableObject::Status &wrsStat)
{
    return false;
}

Memory::Streams::StreamableObject::Status Content_Chunked_SubParser::write(const void *buf, const size_t &count, Memory::Streams::StreamableObject::Status &wrStat)
{
    Memory::Streams::StreamableObject::Status cur;
    char strhex[32];

    if (count+64<count) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }
    snprintf(strhex,sizeof(strhex), pos == 0?"%X\r\n":"\r\n%X\r\n", (unsigned int)count);

    if (!(cur+=dst->writeString(strhex,wrStat)).succeed) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }
    if (!(cur+=dst->writeFullStream(buf,count,wrStat)).succeed) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }

    pos+=count;

    return cur;
}

bool Content_Chunked_SubParser::endBuffer()
{
    Memory::Streams::StreamableObject::Status cur;
    return (cur=dst->writeString(pos == 0? "0\r\n\r\n" : "\r\n0\r\n\r\n",cur)).succeed;
}
