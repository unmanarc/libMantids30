#ifndef MIME_PARTMESSAGE_H
#define MIME_PARTMESSAGE_H

#include "mime_sub_content.h"
#include "mime_sub_header.h"

#include <mdz_mem_vars/streamableobject.h>

namespace Mantids { namespace Protocols { namespace MIME {

class MIME_PartMessage
{
public:
    MIME_PartMessage();

    bool stream(Memory::Streams::StreamableObject::Status &wrStat);

    MIME_Sub_Content * getContent();
    MIME_Sub_Header * getHeader();

private:
    MIME_Sub_Content content;
    MIME_Sub_Header header;
};

}}}

#endif // MIME_PARTMESSAGE_H
