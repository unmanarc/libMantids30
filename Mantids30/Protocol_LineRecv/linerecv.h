#pragma once

#include "linerecv_subparser.h"
#include <Mantids30/Memory/parser.h>

namespace Mantids30::Network::Protocol::Line2Line {

class LineRecv : public Memory::Streams::Parser
{
public:
    LineRecv(std::shared_ptr<StreamableObject> sobject);
    ~LineRecv() override = default;
    void setMaxLineSize(const uint32_t &maxLineSize);

protected:
    virtual bool processParsedLine(const std::string &line) = 0;

    bool initProtocol() override;
    void endProtocol() override {}
    bool changeToNextParser() override;
    LineRecv_SubParser m_subParser;
};

} // namespace Mantids30::Network::Protocol::Line2Line
