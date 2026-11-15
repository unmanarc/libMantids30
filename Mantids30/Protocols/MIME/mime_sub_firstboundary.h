#pragma once

#include <Mantids30/Core/Memory/subparser.h>
#include <string>

namespace Mantids30::Protocols::MIME {

class MIME_Sub_FirstBoundary : public Mantids30::Core::Memory::Streams::SubParser
{
public:
    MIME_Sub_FirstBoundary();

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

protected:
    Mantids30::Core::Memory::Streams::SubParser::ParseResult parse() override;

private:
    std::string m_boundary;
};

} // namespace Mantids30::Protocols::MIME
