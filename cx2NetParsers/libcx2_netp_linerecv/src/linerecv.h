#ifndef HTTP1BASE_H
#define HTTP1BASE_H

#include <cx2_mem_streamparser/streamparser.h>
#include <netinet/in.h>
#include "linerecv_subparser.h"

namespace CX2 { namespace Network { namespace Line2Line {

class LineRecv : public Memory::Streams::Parsing::Parser
{
public:
    LineRecv(Memory::Streams::Streamable *sobject);
    virtual ~LineRecv()  override {}
    void setMaxLineSize(const uint32_t & maxLineSize);

protected:
    virtual bool processParsedLine(const std::string & line) =0;

    virtual bool initProtocol() override;
    virtual void endProtocol() override {}
    virtual bool changeToNextParser() override;
    LineRecv_SubParser subParser;
};

}}}

#endif // HTTP1BASE_H
