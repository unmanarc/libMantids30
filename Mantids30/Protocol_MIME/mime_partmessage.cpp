#include "mime_partmessage.h"

using namespace Mantids30::Network::Protocols::MIME;


bool MIME_PartMessage::stream(Memory::Streams::StreamableObject::Status &wrStat)
{
    if (!m_header.stream(wrStat)) return false;
    if (!m_content.stream(wrStat)) return false;
    return true;
}


MIME_Sub_Content *MIME_PartMessage::getContent()
{
    return &m_content;
}

MIME_Sub_Header * MIME_PartMessage::getHeader()
{
    return &m_header;
}

