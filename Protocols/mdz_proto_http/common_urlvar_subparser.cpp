#include "common_urlvar_subparser.h"

#include "streamdecoder_url.h"

using namespace Mantids::Protocols::HTTP;
using namespace Mantids::Protocols::HTTP::Common;
using namespace Mantids;

URLVar_SubParser::URLVar_SubParser()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"=","&"});
    setMaxObjectSize(4096);
    pData = new Memory::Containers::B_Chunks;
}

URLVar_SubParser::~URLVar_SubParser()
{
    if (pData) delete pData;
}

bool URLVar_SubParser::stream(Memory::Streams::StreamableObject::Status &)
{
    // NOT IMPLEMENTED.
    return false;
}

void URLVar_SubParser::setVarType(bool varName)
{
    if (varName)
        setParseMultiDelimiter({"=","&"}); // Parsing name...
    else
        setParseMultiDelimiter({"&"}); // Parsing value...
}

void URLVar_SubParser::setMaxObjectSize(const uint32_t &size)
{
    setParseDataTargetSize(size);
}

Memory::Containers::B_Chunks *URLVar_SubParser::flushRetrievedContentAsBC()
{
    Memory::Containers::B_Chunks * r = pData;
    pData = new Memory::Containers::B_Chunks;
    return r;
}

std::string URLVar_SubParser::flushRetrievedContentAsString()
{
    std::string r = pData->toString();
    delete pData;
    pData = new Memory::Containers::B_Chunks;
    return r;
}

Memory::Streams::SubParser::ParseStatus URLVar_SubParser::parse()
{
    pData->clear();
    if (!getParsedData()->size()) return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
    Memory::Streams::StreamableObject::Status cur;
    Memory::Streams::Decoders::URL decUrl(pData);
    if (!(cur=getParsedData()->streamTo(&decUrl,cur)).succeed)
    {
        pData->clear();
    }
    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

Memory::Containers::B_Chunks *URLVar_SubParser::getPData()
{
    return pData;
}
