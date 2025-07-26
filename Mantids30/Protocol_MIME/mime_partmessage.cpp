#include "mime_partmessage.h"

using namespace Mantids30::Network::Protocols::MIME;


bool MIME_PartMessage::streamToSubParsers()
{
    if (!m_header.streamToUpstream())
        return false;
    if (!m_content.streamToUpstream())
        return false;

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

