#include "common_content_chunked_subparser.h"
#include <stdio.h>
#include <string.h>

using namespace CX2::Network::HTTP;
using namespace CX2::Network::HTTP::Common;
using namespace CX2;

Content_Chunked_SubParser::Content_Chunked_SubParser(Memory::Streams::Streamable *dst)
{
    this->dst = dst;
    pos = 0;
}

Content_Chunked_SubParser::~Content_Chunked_SubParser()
{
    endBuffer();
}

bool Content_Chunked_SubParser::streamTo(Memory::Streams::Streamable *out, Memory::Streams::Status &wrsStat)
{
    return false;
}

Memory::Streams::Status Content_Chunked_SubParser::write(const void *buf, const size_t &count, Memory::Streams::Status &wrStat)
{
    Memory::Streams::Status cur;
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
    Memory::Streams::Status cur;
    return (cur=dst->writeString(pos == 0? "0\r\n\r\n" : "\r\n0\r\n\r\n",cur)).succeed;
}
