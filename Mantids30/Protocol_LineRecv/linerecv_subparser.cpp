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


void LineRecv_SubParser::setMaxObjectSize(const uint32_t &size)
{
    // TODO: test max limits...
    setParseDataTargetSize(size);
}


std::string LineRecv_SubParser::getParsedString() const
{
    return m_parsedString;
}

Memory::Streams::SubParser::ParseStatus LineRecv_SubParser::parse()
{
    m_parsedString = getParsedBuffer()->toStringEx();
    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
}
