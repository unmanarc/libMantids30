#pragma once

#include <Mantids30/Memory/subparser.h>
#include <string>

namespace Mantids30::Network::Protocols {
namespace MIME {

class MIME_Sub_FirstBoundary : public Memory::Streams::SubParser
{
public:
    MIME_Sub_FirstBoundary();

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    std::string m_boundary;
};

} // namespace MIME
} // namespace Mantids30::Network::Protocols
