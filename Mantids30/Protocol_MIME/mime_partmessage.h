#pragma once

#include "mime_sub_content.h"
#include "mime_sub_header.h"

#include <Mantids30/Memory/streamable_object.h>

namespace Mantids30::Network::Protocols::MIME {

class MIME_PartMessage
{
public:
    MIME_PartMessage() = default;

    bool streamToSubParsers();

    MIME_Sub_Content *getContent();
    MIME_Sub_Header *getHeader();

private:
    MIME_Sub_Content m_content;
    MIME_Sub_Header m_header;
};

} // namespace Mantids30::Network::Protocols::MIME
