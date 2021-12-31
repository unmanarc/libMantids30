#include "linerecv.h"

using namespace Mantids::Network::Line2Line;

LineRecv::LineRecv(Memory::Streams::Streamable *sobject) : Memory::Streams::Parsing::Parser(sobject,false)
{
    initialized = initProtocol();
    currentParser = (Memory::Streams::Parsing::SubParser *)(&subParser);
}

void LineRecv::setMaxLineSize(const uint32_t &maxLineSize)
{
    subParser.setMaxObjectSize(maxLineSize);
}

bool LineRecv::initProtocol()
{
    return true;
}

bool LineRecv::changeToNextParser()
{
    if (!processParsedLine( subParser.getParsedString() ))
    {
        currentParser = nullptr;
        return false;
    }
    return true;
}


