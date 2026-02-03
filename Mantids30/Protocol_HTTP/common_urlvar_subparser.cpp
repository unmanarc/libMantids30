#include "common_urlvar_subparser.h"

#include "streamdecoder_url.h"
#include <memory>
#include <optional>

using namespace Mantids30::Network::Protocols;

using namespace Mantids30;

HTTP::URLVarContent::URLVarContent()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_MULTIDELIMITER);
    setParseMultiDelimiter({"=","&"});
    setMaxObjectSize(4096);
    m_pData = std::make_shared<Memory::Containers::B_Chunks>();
    m_subParserName = "URLVarContent";
}

HTTP::URLVarContent::~URLVarContent()
{
}

void HTTP::URLVarContent::setVarType(bool varName)
{
    if (varName)
        setParseMultiDelimiter({"=","&"}); // Parsing name...
    else
        setParseMultiDelimiter({"&"}); // Parsing value...
}

void HTTP::URLVarContent::setMaxObjectSize(const size_t &size)
{
    setParseDataTargetSize(size);
}

std::shared_ptr<Memory::Containers::B_Chunks> HTTP::URLVarContent::getContentAndFlush()
{
    std::shared_ptr<Memory::Containers::B_Chunks> r = m_pData;
    m_pData = std::make_shared<Memory::Containers::B_Chunks>();
    return r;
}

std::string HTTP::URLVarContent::getContentAsStringAndFlush()
{
    std::optional<std::string> r = m_pData->toString();
    m_pData = std::make_shared<Memory::Containers::B_Chunks>();
    return r?*r:"";
}

Memory::Streams::SubParser::ParseStatus HTTP::URLVarContent::parse()
{
    m_pData->clear();
    if (!getParsedBuffer()->size())
    {
        return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
    }

    Memory::Streams::Decoders::URL urlDecoder;
    urlDecoder.transform(getParsedBuffer(), // Parsed Buffer.
                         m_pData.get() // pData.
                         );

    if (!m_pData->writeStatus.succeed)
    {
        // Error parsing the URLVar...
        m_pData->clear();
        return Memory::Streams::SubParser::PARSE_ERROR;
    }

    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
}

std::shared_ptr<Memory::Containers::B_Chunks> HTTP::URLVarContent::getCurrentContentData()
{
    return m_pData;
}
