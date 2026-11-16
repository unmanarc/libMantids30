#include "mime_sub_content.h"
#include <Mantids30/Core/Memory/subparser.h>
#include <memory>

using namespace Mantids30::Protocol::MIME;
using namespace Mantids30;

#ifdef _WIN32
#include <windows.h>
#endif

MIME_Sub_Content::MIME_Sub_Content()
{
#ifdef _WIN32
    char tempPath[MAX_PATH + 1];
    GetTempPathA(MAX_PATH, tempPath);
    setFsTmpFolder(tempPath);
#else
    setFsTmpFolder("/tmp");
#endif

    replaceContentContainer(std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>());
    setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DIRECT_DELIMITER);
    setBoundary("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    m_subParserName = "MIME_Sub_Content";
}

bool MIME_Sub_Content::streamToUpstream()
{
    // TODO: interpret content encoding...
    if (!m_contentContainer->streamTo(m_upStream))
    {
        return false;
    }

    m_upStream->writeString("\r\n--" + m_boundary);

    return m_upStream->writeStatus.succeed;
}

size_t MIME_Sub_Content::getMaxContentSize() const
{
    return m_maxContentSize;
}

void MIME_Sub_Content::setMaxContentSize(const size_t &value)
{
    m_maxContentSize = value;
    setParseDataTargetSize(m_maxContentSize + m_boundary.size() + 4);
}

size_t MIME_Sub_Content::getMaxContentSizeUntilGoingToFS() const
{
    return m_maxContentSizeUntilGoingToFS;
}

void MIME_Sub_Content::setMaxContentSizeUntilGoingToFS(const size_t &value)
{
    m_maxContentSizeUntilGoingToFS = value;
}

std::string MIME_Sub_Content::getFsTmpFolder() const
{
    return m_fsTmpFolder;
}

void MIME_Sub_Content::setFsTmpFolder(const std::string &value)
{
    m_fsTmpFolder = value;
}

Mantids30::Core::Memory::Streams::SubParser::ParseResult MIME_Sub_Content::parse()
{
    // TODO: interpret content encoding...
    if (getParsedBuffer()->appendTo(*m_contentContainer) != getParsedBuffer()->size())
    {
        return ParseResult::ERROR_STREAM_WRITE;
    }
    if (!getFoundDelimiter().empty())
    {
        // finished (delimiter found).
#ifdef DEBUG
        printf("%p MIME_Sub_Content: Delimiter %s received.\n", this, boundary.c_str());
        fflush(stdout);
#endif
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
    }
#ifdef DEBUG
    printf("%p MIME_Sub_Content: requesting more data.\n", this);
    fflush(stdout);
#endif
    return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
}

std::string MIME_Sub_Content::getBoundary() const
{
    return m_boundary;
}

void MIME_Sub_Content::setBoundary(const std::string &value)
{
    m_boundary = value;
    setParseDelimiter("\r\n--" + m_boundary);
    setParseDataTargetSize(m_maxContentSize + m_boundary.size() + 4);
}

std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> MIME_Sub_Content::getContentContainer() const
{
    return m_contentContainer;
}

void MIME_Sub_Content::replaceContentContainer(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &value)
{
    m_contentContainer = value;
}
