#pragma once

#include <string>
#include <Mantids30/Memory/subparser.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {

class MIME_Sub_FirstBoundary : public Memory::Streams::SubParser
{
public:
    MIME_Sub_FirstBoundary();

    bool streamToUpstream( Memory::Streams::WriteStatus &wrStat) override;

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
private:
    std::string m_boundary;
};

}}}}

