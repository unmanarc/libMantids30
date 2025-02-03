#include "linerecv.h"
#include <memory>

using namespace Mantids30::Network::Protocols::Line2Line;

LineRecv::LineRecv(std::shared_ptr<Memory::Streams::StreamableObject> sobject) : Memory::Streams::Parser(sobject,false)
{
    m_initialized = initProtocol();
    m_currentParser = (Memory::Streams::SubParser *)(&m_subParser);
}

void LineRecv::setMaxLineSize(const uint32_t &maxLineSize)
{
    m_subParser.setMaxObjectSize(maxLineSize);
}

bool LineRecv::initProtocol()
{
    return true;
}

bool LineRecv::changeToNextParser()
{
    if (!processParsedLine( m_subParser.getParsedString() ))
    {
        m_currentParser = nullptr;
        return false;
    }
    return true;
}


