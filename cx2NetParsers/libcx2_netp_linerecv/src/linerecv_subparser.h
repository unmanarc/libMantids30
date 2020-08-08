#ifndef LINERECV_SUBPARSER_H
#define LINERECV_SUBPARSER_H

#include <cx2_mem_streamparser/substreamparser.h>

namespace CX2 { namespace Network { namespace HTTP {

class LineRecv_SubParser : public Memory::Streams::Parsing::SubParser
{
public:
    LineRecv_SubParser();
    ~LineRecv_SubParser() override;

    void setMaxObjectSize(const uint32_t &size);
    bool stream(Memory::Streams::Status &) override;

    std::string getParsedString() const;

protected:
    std::string parsedString;
    Memory::Streams::Parsing::ParseStatus parse() override;

};
}}}

#endif // LINERECV_SUBPARSER_H
