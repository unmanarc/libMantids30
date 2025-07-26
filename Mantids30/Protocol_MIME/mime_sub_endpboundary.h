#pragma once

#include <Mantids30/Memory/subparser.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {


#define ENDP_STAT_UNINITIALIZED -1
#define ENDP_STAT_CONTINUE 0
#define ENDP_STAT_END 1
#define ENDP_STAT_ERROR 2

class MIME_Sub_EndPBoundary : public Memory::Streams::SubParser
{
public:
    MIME_Sub_EndPBoundary();

    void reset();
    int getStatus() const;

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    int m_status;
};

}}}}

