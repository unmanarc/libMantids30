#include "linerecv.h"

using namespace Mantids29::Network::Protocols::Line2Line;

LineRecv::LineRecv(Memory::Streams::StreamableObject *sobject) : Memory::Streams::Parser(sobject,false)
{
    m_initialized = initProtocol();
    m_currentParser = (Memory::Streams::SubParser *)(&subParser);
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
        m_currentParser = nullptr;
        return false;
    }
    return true;
}


