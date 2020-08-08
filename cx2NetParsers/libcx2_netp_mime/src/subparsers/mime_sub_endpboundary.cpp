#include "mime_sub_endpboundary.h"
using namespace CX2::Network::HTTP;
using namespace CX2;

MIME_Sub_EndPBoundary::MIME_Sub_EndPBoundary()
{
    status = ENDP_STAT_UNINITIALIZED;
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_SIZE);
    setParseDataTargetSize(2);
}

bool MIME_Sub_EndPBoundary::stream(Memory::Streams::Status & wrStat)
{
    return true;
}

Memory::Streams::Parsing::ParseStatus MIME_Sub_EndPBoundary::parse()
{
    if (getParsedData()->compare("--",2))
    {
        // END.
        status = ENDP_STAT_END;
        return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }
    if (getParsedData()->compare("\r\n",2))
    {
        status = ENDP_STAT_CONTINUE;
        return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }

    status = ENDP_STAT_ERROR;
    return Memory::Streams::Parsing::PARSE_STAT_ERROR;
}

int MIME_Sub_EndPBoundary::getStatus() const
{
    return status;
}
