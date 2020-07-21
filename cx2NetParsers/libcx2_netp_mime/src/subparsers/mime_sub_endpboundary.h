#ifndef MIME_SUB_ENDPBOUNDARY_H
#define MIME_SUB_ENDPBOUNDARY_H

#include <cx2_mem_streamparser/substreamparser.h>

namespace CX2 { namespace Network { namespace Parsers {


#define ENDP_STAT_UNINITIALIZED -1
#define ENDP_STAT_CONTINUE 0
#define ENDP_STAT_END 1
#define ENDP_STAT_ERROR 2

class MIME_Sub_EndPBoundary : public Memory::Streams::Parsing::SubParser
{
public:
    MIME_Sub_EndPBoundary();
    bool stream(Memory::Streams::Status &wrStat) override;
    int getStatus() const;

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    int status;
};

}}}

#endif // MIME_SUB_ENDPBOUNDARY_H
