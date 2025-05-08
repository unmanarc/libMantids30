#include "common_content.h"
#include <Mantids30/Memory/streamablejson.h>
#include "common_content_chunked_subparser.h"

#include <stdexcept>

// TODO: CHECK THIS CLASS
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Network::Protocols::HTTP::Common;
using namespace Mantids30::Network;

using namespace Mantids30;

Content::Content()
{
    m_subParserName = "Content";
}

bool Content::isDefaultStreamableObj()
{
    return m_usingInternalOutStream;
}

void Content::setSecurityMaxPostDataSize(const uint64_t &value)
{
    m_securityMaxPostDataSize = value;
}

Memory::Streams::SubParser::ParseStatus Content::parse()
{
    switch (m_currentMode)
    {
        case PROCMODE_CHUNK_SIZE:
        {
            size_t targetChunkSize;
            if ((targetChunkSize=parseHttpChunkSize())!=UINT32_MAX)
            {
                if (targetChunkSize>0)
                {
                    setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
                    setParseDataTargetSize(targetChunkSize);
                    m_currentMode = PROCMODE_CHUNK_DATA;
                    return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
                }
                else
                {
                    // Done... last chunk.
                    // report that is last chunk.
                    m_outStream->writeEOF(true);
                    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
                }
            }
            return Memory::Streams::SubParser::PARSE_ERROR;
        }
        case PROCMODE_CHUNK_DATA:
        {
            // Ok, continue
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
            setParseDataTargetSize(2); // for CRLF.
            m_currentMode = PROCMODE_CHUNK_CRLF;
            // Proccess chunk into mem...
            // TODO: validate when outstream is filled up.
            getParsedBuffer()->appendTo(*m_outStream);
            return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
        }
        case PROCMODE_CHUNK_CRLF:
        {
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
            setParseDelimiter("\r\n");
            setParseDataTargetSize(1*KB_MULT); // !1kb max (until fails).
            m_currentMode = PROCMODE_CHUNK_SIZE;
            return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
        }
        case PROCMODE_CONTENT_LENGTH:
        {
            // TODO: validate when outstream is filled up.
            getParsedBuffer()->appendTo(*m_outStream);
            if (getUnparsedDataSize()>0)
            {
#ifdef DEBUG
                printf("%p Content PROCMODE_CONTENT_LENGTH - left to parse: %lu\n",this, getUnparsedDataSize());
#endif
                return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
            }
            else
            {
#ifdef DEBUG
                printf("%p Content PROCMODE_CONTENT_LENGTH - EOS\n",this);
#endif
                // End of stream reached...
                m_outStream->writeEOF(true);
                return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
            }
        }
        case PROCMODE_CONNECTION_CLOSE:
        {
            // Content satisfied...
            if (getParsedBuffer()->size())
            {
                // Parsing data...
                // TODO: validate when outstream is filled up.
                getParsedBuffer()->appendTo(*m_outStream);
                return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
            }
            else
            {
                // No more data to parse, ending and processing it...
                m_outStream->writeEOF(true);
                return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
            }
        }
    }
    return Memory::Streams::SubParser::PARSE_ERROR;
}

uint32_t Content::parseHttpChunkSize()
{
    uint32_t parsedSize = getParsedBuffer()->toUInt32(16);
    if ( parsedSize == UINT32_MAX || parsedSize>m_securityMaxHttpChunkSize)
        return UINT32_MAX;
    else
        return parsedSize;
}

Content::eTransmitionMode Content::getTransmitionMode() const
{
    return m_transmitionMode;
}

std::shared_ptr<URLVars> Content::getUrlPostVars()
{
    if (m_containerType == CONTENT_TYPE_URL && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<URLVars>(m_outStream);
    throw std::runtime_error("Invalid operation: getUrlPostVars should not be called when the content type is not URL.");
}

std::shared_ptr<Memory::Streams::StreamableJSON> Content::getJSONVars()
{
    if (m_containerType == CONTENT_TYPE_JSON && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<Memory::Streams::StreamableJSON>(m_outStream);
    throw std::runtime_error("Invalid operation: getJSONVars should not be called when the content type is not JSON.");
}

std::shared_ptr<Protocols::MIME::MIME_Message> Content::getMultiPartVars()
{
    if (m_containerType == CONTENT_TYPE_MIME && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<Protocols::MIME::MIME_Message>(m_outStream);
    throw std::runtime_error("Invalid operation: getMultiPartVars should not be called when the content type is not MIME.");
}

Content::eDataType Content::getContainerType() const
{
    return m_containerType;
}

std::shared_ptr<Memory::Abstract::Vars> Content::postVars()
{
    if (!m_usingInternalOutStream)
    {
        throw std::runtime_error("Error: not using internal output stream");
    }
    if (m_containerType == CONTENT_TYPE_BIN)
    {
        throw std::runtime_error("Error: container type is binary");
    }
    if (m_containerType == CONTENT_TYPE_JSON)
    {
        throw std::runtime_error("Error: container type is JSON");
    }

    std::shared_ptr<Memory::Abstract::Vars> vars = std::dynamic_pointer_cast<Memory::Abstract::Vars>(m_outStream);
    if (!vars)
    {
        throw std::runtime_error("Error: outStream cannot be cast to Vars");
    }
    return vars;
}

void Content::setContainerType(const eDataType &value)
{
    m_containerType = value;
    if (m_usingInternalOutStream)
    {
        switch (m_containerType)
        {
        case CONTENT_TYPE_MIME:
            m_outStream = MIME::MIME_Message::create();
            break;
        case CONTENT_TYPE_URL:
            m_outStream = URLVars::create();
            break;
        case CONTENT_TYPE_JSON:
            m_outStream = std::make_shared<Mantids30::Memory::Streams::StreamableJSON>();
            break;
        case CONTENT_TYPE_BIN:
            m_outStream = std::make_shared<Memory::Containers::B_Chunks>();
            break;
        }
    }
}

void Content::setSecurityMaxHttpChunkSize(const uint32_t &value)
{
    m_securityMaxHttpChunkSize = value;
}

bool Content::streamToUpstream( Memory::Streams::WriteStatus & wrStat)
{
    // Act as a client. Send data from here.
    switch (m_transmitionMode)
    {
    case TRANSMIT_MODE_CHUNKS:
    {
        Content_Chunked_SubParser retr(m_upStream);
        return m_outStream->streamTo(&retr,wrStat) && m_upStream->getFailedWriteState()==0;
    }
    case TRANSMIT_MODE_CONTENT_LENGTH:
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        return m_outStream->streamTo(m_upStream, wrStat) && m_upStream->getFailedWriteState()==0;
    }
    }
    return true;
}

void Content::setTransmitionMode(const eTransmitionMode &value)
{
    m_transmitionMode = value;

    switch (m_transmitionMode)
    {
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        // TODO: disable connection-close for client->server
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DIRECT);
        setParseDataTargetSize(m_securityMaxPostDataSize); // !1kb max (until fails).
        m_currentMode = PROCMODE_CONNECTION_CLOSE;
    }break;
    case TRANSMIT_MODE_CHUNKS:
    {
        // TODO: disable chunk transmition for client->server
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(64); // 64 bytes max (until fails).
        m_currentMode = PROCMODE_CHUNK_SIZE;
    }break;
    case TRANSMIT_MODE_CONTENT_LENGTH:
    {
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DIRECT);
        m_currentMode = PROCMODE_CONTENT_LENGTH;
    }break;
    }
}

bool Content::setContentLenSize(const uint64_t &contentLengthSize)
{
    if (contentLengthSize>m_securityMaxPostDataSize)
    {
        // Can't receive this data...
        m_currentContentLengthSize = 0;
        setParseDataTargetSize(0);
        return false;
    }

    // The content length specified in the header
    m_currentContentLengthSize=contentLengthSize;
    setParseDataTargetSize(contentLengthSize);

    return true;
}

void Content::setMaxBinPostMemoryBeforeFS(const uint64_t& value)
{
    if (m_containerType == CONTENT_TYPE_BIN && m_usingInternalOutStream)
    {
        std::dynamic_pointer_cast<Memory::Containers::B_Chunks>(m_outStream)->setMaxContainerSizeUntilGoingToFS(value);
    }
    else
    {
        throw std::runtime_error("This method is only meant for the internal container. Custom containers should be configured externally.");
    }
}
/*
void Content::useFilesystem(const std::string &fsFilePath)
{
    // TODO:
    //binDataContainer.setFsBaseFileName();
}
*/

std::shared_ptr<Memory::Streams::StreamableObject> Content::getStreamableObj()
{
    return m_outStream;
}

void Content::setStreamableObj(std::shared_ptr<Memory::Streams::StreamableObject> outDataContainer)
{
    // This stream has been setted up before...
    this->m_usingInternalOutStream = false;

    // The previous stream is destroyed after the assignation...
    this->m_outStream = outDataContainer;

    if (!outDataContainer)
    {
        // Point to empty container...
        this->m_usingInternalOutStream = true;
        // Create a new streamable object with the last content type...
        setContainerType(m_containerType);
    }
}

uint64_t Content::getStreamSize()
{
    return m_outStream->size();
}

