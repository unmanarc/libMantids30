#include "mime_sub_firstboundary.h"
using namespace Mantids::Protocols::MIME;
using namespace Mantids;

MIME_Sub_FirstBoundary::MIME_Sub_FirstBoundary()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setBoundary("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
}

bool MIME_Sub_FirstBoundary::stream(Memory::Streams::StreamableObject::Status & wrStat)
{
    return true;
}

std::string MIME_Sub_FirstBoundary::getBoundary() const
{
    return boundary;
}

void MIME_Sub_FirstBoundary::setBoundary(const std::string &value)
{
    setParseDelimiter("--" + value + "\r\n");
    setParseDataTargetSize(value.size()+4);
    boundary = value;
}

Memory::Streams::SubParser::ParseStatus MIME_Sub_FirstBoundary::parse()
{
    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}
