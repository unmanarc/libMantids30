#include "mime_sub_firstboundary.h"
using namespace Mantids30::Network::Protocols::MIME;
using namespace Mantids30;

MIME_Sub_FirstBoundary::MIME_Sub_FirstBoundary()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setBoundary("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    m_subParserName = "MIME_Sub_FirstBoundary";
}


std::string MIME_Sub_FirstBoundary::getBoundary() const
{
    return m_boundary;
}

void MIME_Sub_FirstBoundary::setBoundary(const std::string &value)
{
    setParseDelimiter("--" + value + "\r\n");
    // Max 512 bytes of trash before the first boundary...
    setParseDataTargetSize(value.size()+512);
    m_boundary = value;
}

Memory::Streams::SubParser::ParseStatus MIME_Sub_FirstBoundary::parse()
{
#ifdef DEBUG
    printf("Initial Delimiter %s received on MIME First Boundary.\n", boundary.c_str());fflush(stdout);
#endif
    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
}
