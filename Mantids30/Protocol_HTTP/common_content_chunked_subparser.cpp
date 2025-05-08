#include "common_content_chunked_subparser.h"
#include <stdio.h>
#include <string.h>

using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Network::Protocols::HTTP::Common;
using namespace Mantids30;

Content_Chunked_SubParser::Content_Chunked_SubParser(
    StreamableObject *upStreamOut)
{
    this->upStreamOut = upStreamOut;
}

Content_Chunked_SubParser::~Content_Chunked_SubParser()
{
    endBuffer();
}

bool Content_Chunked_SubParser::streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus &wrsStat)
{
    return false;
}

Memory::Streams::WriteStatus Content_Chunked_SubParser::write(const void *buf, const size_t &count, Memory::Streams::WriteStatus &wrStat)
{
    Memory::Streams::WriteStatus cur;
    char strhex[32];

    if (count+64<count) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }
    snprintf(strhex,sizeof(strhex), m_pos == 0?"%X\r\n":"\r\n%X\r\n", (unsigned int)count);

    if (!(cur+=upStreamOut->writeString(strhex,wrStat)).succeed) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }
    if (!(cur+=upStreamOut->writeFullStream(buf,count,wrStat)).succeed) { cur.succeed=wrStat.succeed=setFailedWriteState(); return cur; }

    m_pos+=count;

    return cur;
}

bool Content_Chunked_SubParser::endBuffer()
{
    Memory::Streams::WriteStatus cur;
    return (cur=upStreamOut->writeString(m_pos == 0? "0\r\n\r\n" : "\r\n0\r\n\r\n",cur)).succeed;
}
