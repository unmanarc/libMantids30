#pragma once

#include <Mantids30/Memory/subparser.h>

namespace Mantids30::Network::Protocols::Line2Line {

class LineRecv_SubParser : public Memory::Streams::SubParser
{
public:
    LineRecv_SubParser();
    ~LineRecv_SubParser() override = default;

    void setMaxObjectSize(const uint32_t &size);

    std::string getParsedString() const;

protected:
    std::string m_parsedString;
    Memory::Streams::SubParser::ParseResult parse() override;
};
} // namespace Mantids30::Network::Protocols::Line2Line
