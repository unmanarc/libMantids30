#include "linerecv_subparser.h"

using namespace Mantids30::Network::Protocols::Line2Line;
using namespace Mantids30;

LineRecv_SubParser::LineRecv_SubParser()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"\x0a", "\x0d"});
    setMaxObjectSize(65536);
    m_subParserName = "LineRecv_SubParser";

}

LineRecv_SubParser::~LineRecv_SubParser()
{
}

void LineRecv_SubParser::setMaxObjectSize(const uint32_t &size)
{
    // TODO: test max limits...
    setParseDataTargetSize(size);
}

bool LineRecv_SubParser::stream(Memory::Streams::StreamableObject::Status &)
{
    // NOT IMPLEMENTED.
    return false;
}

std::string LineRecv_SubParser::getParsedString() const
{
    return m_parsedString;
}

Memory::Streams::SubParser::ParseStatus LineRecv_SubParser::parse()
{
    m_parsedString = getParsedBuffer()->toString();
    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}
