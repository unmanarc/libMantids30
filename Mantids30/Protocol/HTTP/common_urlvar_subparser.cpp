#include "common_urlvar_subparser.h"

#include "streamdecoder_url.h"
#include <memory>
#include <optional>

using namespace Mantids30::Protocol;

using namespace Mantids30;

Mantids30::Protocol::HTTP::URLVarContent::URLVarContent()
{
    setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::MULTIDELIMITER);
    setParseMultiDelimiter({"=", "&"});
    setMaxObjectSize(4096);
    m_pData = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
    m_subParserName = "URLVarContent";
}

Mantids30::Protocol::HTTP::URLVarContent::~URLVarContent() = default;

void Mantids30::Protocol::HTTP::URLVarContent::setVarType(bool varName)
{
    if (varName)
    {
        setParseMultiDelimiter({"=", "&"}); // Parsing name...
    }
    else
    {
        setParseMultiDelimiter({"&"}); // Parsing value...
    }
}

void Mantids30::Protocol::HTTP::URLVarContent::setMaxObjectSize(const size_t &size)
{
    setParseDataTargetSize(size);
}

std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> Mantids30::Protocol::HTTP::URLVarContent::getContentAndFlush()
{
    std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> r = m_pData;
    m_pData = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
    return r;
}

std::string Mantids30::Protocol::HTTP::URLVarContent::getContentAsStringAndFlush()
{
    std::optional<std::string> r = m_pData->toString();
    m_pData = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
    return r ? *r : "";
}

Mantids30::Core::Memory::Streams::SubParser::ParseResult Mantids30::Protocol::HTTP::URLVarContent::parse()
{
    m_pData->clear();
    if (!getParsedBuffer()->size())
    {
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
    }

    Mantids30::Protocol::HTTP::Decoders::URL urlDecoder;
    urlDecoder.transform(getParsedBuffer(), // Parsed Buffer.
                         m_pData.get()      // pData.
    );

    if (!m_pData->writeStatus.succeed)
    {
        // Error parsing the URLVar...
        m_pData->clear();
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::ERROR_STREAM_WRITE;
    }

    return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
}

std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> Mantids30::Protocol::HTTP::URLVarContent::getCurrentContentData()
{
    return m_pData;
}
