#include "linerecv_subparser.h"

using namespace Mantids::Protocols::Line2Line;
using namespace Mantids;

LineRecv_SubParser::LineRecv_SubParser()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"\x0a", "\x0d"});
    setMaxObjectSize(65536);
    subParserName = "LineRecv_SubParser";

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
    return parsedString;
}

Memory::Streams::SubParser::ParseStatus LineRecv_SubParser::parse()
{
    parsedString = getParsedBuffer()->toString();
    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}
