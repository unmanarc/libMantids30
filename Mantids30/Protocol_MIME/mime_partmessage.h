#pragma once

#include "mime_sub_content.h"
#include "mime_sub_header.h"

#include <Mantids30/Memory/streamableobject.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {

class MIME_PartMessage
{
public:
    MIME_PartMessage() = default;

    bool stream(Memory::Streams::StreamableObject::Status &wrStat);

    MIME_Sub_Content * getContent();
    MIME_Sub_Header * getHeader();

private:
    MIME_Sub_Content m_content;
    MIME_Sub_Header m_header;
};

}}}}

