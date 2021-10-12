#include "http_urlvarcontent_subparser.h"

#include "streamdecoder_url.h"

using namespace CX2::Network::HTTP;
using namespace CX2;

HTTP_URLVarContent_SubParser::HTTP_URLVarContent_SubParser()
{
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"=","&"});
    setMaxObjectSize(4096);
    pData = new Memory::Containers::B_Chunks;
}

HTTP_URLVarContent_SubParser::~HTTP_URLVarContent_SubParser()
{
    if (pData) delete pData;
}

bool HTTP_URLVarContent_SubParser::stream(Memory::Streams::Status &)
{
    // NOT IMPLEMENTED.
    return false;
}

void HTTP_URLVarContent_SubParser::setVarType(bool varName)
{
    if (varName)
        setParseMultiDelimiter({"=","&"}); // Parsing name...
    else
        setParseMultiDelimiter({"&"}); // Parsing value...
}

void HTTP_URLVarContent_SubParser::setMaxObjectSize(const uint32_t &size)
{
    setParseDataTargetSize(size);
}

Memory::Containers::B_Chunks *HTTP_URLVarContent_SubParser::flushRetrievedContentAsBC()
{
    Memory::Containers::B_Chunks * r = pData;
    pData = new Memory::Containers::B_Chunks;
    return r;
}

std::string HTTP_URLVarContent_SubParser::flushRetrievedContentAsString()
{
    std::string r = pData->toString();
    delete pData;
    pData = new Memory::Containers::B_Chunks;
    return r;
}

Memory::Streams::Parsing::ParseStatus HTTP_URLVarContent_SubParser::parse()
{
    pData->clear();
    if (!getParsedData()->size()) return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
    Memory::Streams::Status cur;
    Memory::Streams::Decoders::URL decUrl(pData);
    if (!(cur=getParsedData()->streamTo(&decUrl,cur)).succeed)
    {
        pData->clear();
    }
    return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

Memory::Containers::B_Chunks *HTTP_URLVarContent_SubParser::getPData()
{
    return pData;
}
