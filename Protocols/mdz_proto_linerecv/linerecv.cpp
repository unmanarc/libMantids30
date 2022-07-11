#include "linerecv.h"

using namespace Mantids::Protocols::Line2Line;

LineRecv::LineRecv(Memory::Streams::StreamableObject *sobject) : Memory::Streams::Parser(sobject,false)
{
    initialized = initProtocol();
    currentParser = (Memory::Streams::SubParser *)(&subParser);
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


