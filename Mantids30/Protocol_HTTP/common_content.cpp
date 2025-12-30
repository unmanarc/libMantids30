#include "common_content.h"
#include <Mantids30/Memory/streamable_json.h>
#include "common_content_chunked_subparser.h"

#include <optional>
#include <stdexcept>

// TODO: CHECK THIS CLASS
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

HTTP::Content::Content()
{
    m_subParserName = "Content";
}

bool HTTP::Content::isDefaultStreamableObj()
{
    return m_usingInternalOutStream;
}

void HTTP::Content::setSecurityMaxPostDataSize(const size_t &value)
{
    m_securityMaxPostDataSize = value;
}

Memory::Streams::SubParser::ParseStatus HTTP::Content::parse()
{
    switch (m_currentMode)
    {
        case PROCMODE_CHUNK_CLOSE:
        {
            // TODO: check if there is data, because may be new headers here...
            m_contentStreamableObject->writeEOF();
            return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
        }
        case PROCMODE_CHUNK_SIZE:
        {
            std::optional<size_t> targetChunkSize;
            if ((targetChunkSize=parseHttpChunkSize())!=std::nullopt)
            {
                if (targetChunkSize>0)
                {
                    setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
                    setParseDataTargetSize(*targetChunkSize);
                    m_currentMode = PROCMODE_CHUNK_DATA;
                    return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
                }
                else
                {
                    // Done... last chunk. Get the last \r\n here..
                    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
                    setParseDelimiter("\r\n");
                    setParseDataTargetSize(64*KB_MULT); // !64kb max (until fails, this may include new headers?).
                    m_currentMode = PROCMODE_CHUNK_CLOSE;
                    return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
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
            getParsedBuffer()->appendTo(*m_contentStreamableObject);
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
            getParsedBuffer()->appendTo(*m_contentStreamableObject);
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
                m_contentStreamableObject->writeEOF();
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
                getParsedBuffer()->appendTo(*m_contentStreamableObject);
                return Memory::Streams::SubParser::PARSE_GET_MORE_DATA;
            }
            else
            {
                // No more data to parse, ending and processing it...
                m_contentStreamableObject->writeEOF();
                return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
            }
        }
    }
    return Memory::Streams::SubParser::PARSE_ERROR;
}

std::optional<uint32_t> HTTP::Content::parseHttpChunkSize()
{
    std::optional<uint32_t> parsedSize = getParsedBuffer()->toUInt32(16);

    if (parsedSize==std::nullopt || parsedSize.value()>m_securityMaxHttpChunkSize)
    {
        return std::nullopt;
    }

    return *parsedSize;
}

HTTP::Content::eTransmitionMode HTTP::Content::getTransmitionMode() const
{
    return m_transmitionMode;
}

std::shared_ptr<HTTP::URLVars> HTTP::Content::getUrlPostVars()
{
    if (m_containerType == CONTENT_TYPE_URL && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<URLVars>(m_contentStreamableObject);
    throw std::runtime_error("Invalid operation: getUrlPostVars should not be called when the content type is not URL.");
}

std::shared_ptr<Memory::Streams::StreamableJSON> HTTP::Content::getJSONVars()
{
    if (m_containerType == CONTENT_TYPE_JSON && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<Memory::Streams::StreamableJSON>(m_contentStreamableObject);
    throw std::runtime_error("Invalid operation: getJSONVars should not be called when the content type is not JSON.");
}

std::shared_ptr<Protocols::MIME::MIME_Message> HTTP::Content::getMultiPartVars()
{
    if (m_containerType == CONTENT_TYPE_MIME && m_usingInternalOutStream )
        return std::dynamic_pointer_cast<Protocols::MIME::MIME_Message>(m_contentStreamableObject);
    throw std::runtime_error("Invalid operation: getMultiPartVars should not be called when the content type is not MIME.");
}

HTTP::Content::eDataType HTTP::Content::getContainerType() const
{
    return m_containerType;
}

std::shared_ptr<Memory::Abstract::Vars> HTTP::Content::postVars()
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

    std::shared_ptr<Memory::Abstract::Vars> vars = std::dynamic_pointer_cast<Memory::Abstract::Vars>(m_contentStreamableObject);
    if (!vars)
    {
        throw std::runtime_error("Error: outStream cannot be cast to Vars");
    }
    return vars;
}

void HTTP::Content::setContainerType(const HTTP::Content::eDataType &value)
{
    m_containerType = value;
    if (m_usingInternalOutStream)
    {
        switch (m_containerType)
        {
        case CONTENT_TYPE_MIME:
            m_contentStreamableObject = MIME::MIME_Message::create();
            break;
        case CONTENT_TYPE_URL:
            m_contentStreamableObject = URLVars::create();
            break;
        case CONTENT_TYPE_JSON:
            m_contentStreamableObject = std::make_shared<Mantids30::Memory::Streams::StreamableJSON>();
            break;
        case CONTENT_TYPE_BIN:
            m_contentStreamableObject = std::make_shared<Memory::Containers::B_Chunks>();
            break;
        }
    }
}

void HTTP::Content::setSecurityMaxHttpChunkSize(const uint32_t &value)
{
    m_securityMaxHttpChunkSize = value;
}

bool HTTP::Content::streamToUpstream()
{
    // Act as a client. Send data from here.
    switch (m_transmitionMode)
    {
    case TRANSMIT_MODE_CHUNKS:
    {
        ContentChunkedTransformer chunkedLiveTransformer(m_upStream);
        m_contentStreamableObject->streamTo(&chunkedLiveTransformer);
        return m_contentStreamableObject->writeStatus.succeed;
    }
    case TRANSMIT_MODE_CONTENT_LENGTH:
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        m_contentStreamableObject->streamTo(m_upStream);
        return m_contentStreamableObject->writeStatus.succeed;
    }
    }
    return true;
}

void HTTP::Content::setTransmitionMode(const eTransmitionMode &value)
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

bool HTTP::Content::setContentLengthSize(const size_t &contentLengthSize)
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

void HTTP::Content::setMaxBinPostMemoryBeforeFS(const size_t& value)
{
    if (m_containerType == CONTENT_TYPE_BIN && m_usingInternalOutStream)
    {
        std::dynamic_pointer_cast<Memory::Containers::B_Chunks>(m_contentStreamableObject)->setMaxSizeInMemoryBeforeMovingToDisk(value);
    }
    else
    {
        throw std::runtime_error("This method is only meant for the internal container. Custom containers should be configured externally.");
    }
}

std::shared_ptr<Memory::Streams::StreamableObject> HTTP::Content::getStreamableObject()
{
    return m_contentStreamableObject;
}

void HTTP::Content::setStreamableObj(std::shared_ptr<Memory::Streams::StreamableObject> outDataContainer)
{
    // This stream has been setted up before...
    this->m_usingInternalOutStream = false;

    // The previous stream is destroyed after the assignation...
    this->m_contentStreamableObject = outDataContainer;

    if (!outDataContainer)
    {
        // Point to empty container...
        this->m_usingInternalOutStream = true;
        // Create a new streamable object with the last content type...
        setContainerType(m_containerType);
    }
}

size_t HTTP::Content::getStreamSize()
{
    if (!m_contentStreamableObject)
        return 0;
    return m_contentStreamableObject->size();
}

