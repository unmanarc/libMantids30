#include "common_urlvar_subparser.h"

#include "streamdecoder_url.h"
#include <memory>

using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Network::Protocols::HTTP::Common;
using namespace Mantids30;

URLVar_SubParser::URLVar_SubParser()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"=","&"});
    setMaxObjectSize(4096);
    pData = std::make_shared<Memory::Containers::B_Chunks>();

    subParserName = "URLVar_SubParser";

}

URLVar_SubParser::~URLVar_SubParser()
{
    //if (pData) delete pData;
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

std::shared_ptr<Memory::Containers::B_Chunks> URLVar_SubParser::flushRetrievedContentAsBC()
{
    std::shared_ptr<Memory::Containers::B_Chunks> r = pData;
    pData = std::make_shared<Memory::Containers::B_Chunks>();
    return r;
}

std::string URLVar_SubParser::flushRetrievedContentAsString()
{
    std::string r = pData->toString();
    //delete pData;
    pData = std::make_shared<Memory::Containers::B_Chunks>();
    return r;
}

Memory::Streams::SubParser::ParseStatus URLVar_SubParser::parse()
{
    pData->clear();
    if (!getParsedBuffer()->size()) return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
    Memory::Streams::StreamableObject::Status cur;
    std::shared_ptr<Memory::Streams::Decoders::URL> decUrl = std::make_shared<Memory::Streams::Decoders::URL>(pData);
    if (!(cur=getParsedBuffer()->streamTo(decUrl,cur)).succeed)
    {
        pData->clear();
    }
    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

std::shared_ptr<Memory::Containers::B_Chunks> URLVar_SubParser::getPData()
{
    return pData;
}
