#ifndef LINERECV_SUBPARSER_H
#define LINERECV_SUBPARSER_H

#include <mdz_mem_vars/subparser.h>

namespace Mantids { namespace Protocols { namespace Line2Line {

class LineRecv_SubParser : public Memory::Streams::SubParser
{
public:
    LineRecv_SubParser();
    ~LineRecv_SubParser() override;

    void setMaxObjectSize(const uint32_t &size);
    bool stream(Memory::Streams::StreamableObject::Status &) override;

    std::string getParsedString() const;

protected:
    std::string parsedString;
    Memory::Streams::SubParser::ParseStatus parse() override;

};
}}}

#endif // LINERECV_SUBPARSER_H
