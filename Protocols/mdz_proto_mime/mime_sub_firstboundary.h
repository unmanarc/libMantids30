#ifndef MIME_SUB_ENDPOINT_H
#define MIME_SUB_ENDPOINT_H

#include <string>
#include <mdz_mem_vars/subparser.h>

namespace Mantids { namespace Protocols { namespace MIME {

class MIME_Sub_FirstBoundary : public Memory::Streams::SubParser
{
public:
    MIME_Sub_FirstBoundary();

    bool stream(Memory::Streams::StreamableObject::Status &wrStat) override;

    std::string getBoundary() const;
    void setBoundary(const std::string &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;
private:
    std::string boundary;
};

}}}

#endif // MIME_SUB_ENDPOINT_H
