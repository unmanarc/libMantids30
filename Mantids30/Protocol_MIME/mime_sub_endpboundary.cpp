#include "mime_sub_endpboundary.h"

using namespace Mantids30::Network::Protocols::MIME;
using namespace Mantids30;

MIME_Sub_EndPBoundary::MIME_Sub_EndPBoundary()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    reset();
    m_subParserName = "MIME_Sub_EndPBoundary";

}

void MIME_Sub_EndPBoundary::reset()
{
    m_status = ENDP_STAT_UNINITIALIZED;
    setParseMultiDelimiter( { "--\r\n", "\r\n" } );
    setParseDataTargetSize(16);
    clear();
}

bool MIME_Sub_EndPBoundary::streamToUpstream( Memory::Streams::WriteStatus & wrStat)
{
    return true;
}

Memory::Streams::SubParser::ParseStatus MIME_Sub_EndPBoundary::parse()
{
#ifdef DEBUG
    printf("Parsed 2 bytes on MIME EndPBoundary, %lu\n", getParsedBuffer()->size());fflush(stdout);
    BIO_dump_fp (stdout, (char *)getParsedBuffer()->toString().c_str(), getParsedBuffer()->toString().size());
#endif

    if (getFoundDelimiter() == "--\r\n" )
    {
#ifdef DEBUG
        printf("MIME EndPBoundary match with ENDED...\n");fflush(stdout);
#endif
        // END.
        m_status = ENDP_STAT_END;
        return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
    }
    else if (getFoundDelimiter() == "\r\n")
    {
#ifdef DEBUG
        printf("MIME EndPBoundary match with NEXT HEADER...\n");fflush(stdout);
#endif
        m_status = ENDP_STAT_CONTINUE;
        return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
    }
#ifdef DEBUG
    printf("MIME EndPBoundary Failed with unexpected content\n");fflush(stdout);
#endif
    m_status = ENDP_STAT_ERROR;
    return Memory::Streams::SubParser::PARSE_ERROR;
}

int MIME_Sub_EndPBoundary::getStatus() const
{
    return m_status;
}
