#include "linerecv_subparser.h"

using namespace Mantids::Network::Line2Line;
using namespace Mantids;

LineRecv_SubParser::LineRecv_SubParser()
{
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"\x0a", "\x0d"});
    setMaxObjectSize(65536);
}

LineRecv_SubParser::~LineRecv_SubParser()
{
}

void LineRecv_SubParser::setMaxObjectSize(const uint32_t &size)
{
    // TODO: test max limits...
    setParseDataTargetSize(size);
}

bool LineRecv_SubParser::stream(Memory::Streams::Status &)
{
    // NOT IMPLEMENTED.
    return false;
}

std::string LineRecv_SubParser::getParsedString() const
{
    return parsedString;
}

Memory::Streams::Parsing::ParseStatus LineRecv_SubParser::parse()
{
    parsedString = getParsedData()->toString();
    return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}
