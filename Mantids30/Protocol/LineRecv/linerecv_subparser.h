#pragma once

#include <Mantids30/Core/Memory/subparser.h>

namespace Mantids30::Protocol::LineRecv {

class LineRecv_SubParser : public Mantids30::Core::Memory::Streams::SubParser
{
public:
    LineRecv_SubParser();
    ~LineRecv_SubParser() override = default;

    void setMaxObjectSize(const uint32_t &size);

    std::string getParsedString() const;

protected:
    std::string m_parsedString;
    Mantids30::Core::Memory::Streams::SubParser::ParseResult parse() override;
};
} // namespace Mantids30::Protocol::LineRecv
