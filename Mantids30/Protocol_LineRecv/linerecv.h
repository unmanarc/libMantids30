#pragma once

#include "linerecv_subparser.h"
#include <Mantids30/Memory/parser.h>

namespace Mantids30::Network::Protocols {
namespace Line2Line {

class LineRecv : public Memory::Streams::Parser
{
public:
    LineRecv(std::shared_ptr<StreamableObject> sobject);
    virtual ~LineRecv() override = default;
    void setMaxLineSize(const uint32_t &maxLineSize);

protected:
    virtual bool processParsedLine(const std::string &line) = 0;

    virtual bool initProtocol() override;
    virtual void endProtocol() override {}
    virtual bool changeToNextParser() override;
    LineRecv_SubParser m_subParser;
};

} // namespace Line2Line
} // namespace Mantids30::Network::Protocols
