#pragma once

#include <Mantids29/Memory/subparser.h>

namespace Mantids29 { namespace Network { namespace Protocols { namespace MIME {


#define ENDP_STAT_UNINITIALIZED -1
#define ENDP_STAT_CONTINUE 0
#define ENDP_STAT_END 1
#define ENDP_STAT_ERROR 2

class MIME_Sub_EndPBoundary : public Memory::Streams::SubParser
{
public:
    MIME_Sub_EndPBoundary();

    void reset();
    bool stream(Memory::Streams::StreamableObject::Status &wrStat) override;
    int getStatus() const;

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    int status;
};

}}}}

