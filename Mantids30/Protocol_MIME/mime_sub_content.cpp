#include "mime_sub_content.h"
#include <Mantids30/Memory/subparser.h>
#include <memory>

using namespace Mantids30::Network::Protocols::MIME;
using namespace Mantids30;

#ifdef _WIN32
#include <windows.h>
#endif

MIME_Sub_Content::MIME_Sub_Content()
{
#ifdef _WIN32
    char tempPath[MAX_PATH+1];
    GetTempPathA(MAX_PATH,tempPath);
    setFsTmpFolder(tempPath);
#else
    setFsTmpFolder("/tmp");
#endif

    m_contentContainer = nullptr;
    replaceContentContainer(std::make_shared<Memory::Containers::B_Chunks>());
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DIRECT_DELIMITER);
    setBoundary("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    m_subParserName = "MIME_Sub_Content";
}

MIME_Sub_Content::~MIME_Sub_Content()
{
    //if (contentContainer)
//        delete contentContainer;
}

bool MIME_Sub_Content::stream(Memory::Streams::StreamableObject::Status &wrStat)
{
    Memory::Streams::StreamableObject::Status cur;
    // TODO: interpret content encoding...
    if (!m_contentContainer->streamTo(m_upStream,wrStat)) return false;
    if (!(cur+=m_contentContainer->writeString("\r\n--" + m_boundary, wrStat)).succeed) return false;
    return true;
}

uint64_t MIME_Sub_Content::getMaxContentSize() const
{
    return m_maxContentSize;
}

void MIME_Sub_Content::setMaxContentSize(const uint64_t &value)
{
    m_maxContentSize = value;
    setParseDataTargetSize(m_maxContentSize+m_boundary.size()+4);
}

uint64_t MIME_Sub_Content::getMaxContentSizeUntilGoingToFS() const
{
    return m_maxContentSizeUntilGoingToFS;
}

void MIME_Sub_Content::setMaxContentSizeUntilGoingToFS(const uint64_t &value)
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

Memory::Streams::SubParser::ParseStatus MIME_Sub_Content::parse()
{
    // TODO: interpret content encoding...
    getParsedBuffer()->appendTo(*m_contentContainer);
    if (getDelimiterFound().size())
    {
        // finished (delimiter found).
#ifdef DEBUG
        printf("%p MIME_Sub_Content: Delimiter %s received.\n", this, boundary.c_str());fflush(stdout);
#endif
        return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }
#ifdef DEBUG
    printf("%p MIME_Sub_Content: requesting more data.\n", this);fflush(stdout);
#endif
    return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
}

std::string MIME_Sub_Content::getBoundary() const
{
    return m_boundary;
}

void MIME_Sub_Content::setBoundary(const std::string &value)
{
    m_boundary = value;
    setParseDelimiter("\r\n--" + m_boundary);
    setParseDataTargetSize(m_maxContentSize+m_boundary.size()+4);
}

std::shared_ptr<Memory::Streams::StreamableObject> MIME_Sub_Content::getContentContainer() const
{
    return m_contentContainer;
}

void MIME_Sub_Content::replaceContentContainer(std::shared_ptr<Memory::Streams::StreamableObject> value)
{
//    if (contentContainer) delete contentContainer;
    m_contentContainer = value;
}
