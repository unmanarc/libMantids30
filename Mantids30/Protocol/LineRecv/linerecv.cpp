#include "linerecv.h"
#include <memory>

using namespace Mantids30::Protocol::LineRecv;

LineRecv::LineRecv(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &sobject)
    : Mantids30::Core::Memory::Streams::Parser(sobject, false)
{
    m_initialized = initProtocol();
    m_currentSubParser = &m_subParser;
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
    if (!processParsedLine(m_subParser.getParsedString()))
    {
        m_currentSubParser = nullptr;
        return false;
    }
    return true;
}
