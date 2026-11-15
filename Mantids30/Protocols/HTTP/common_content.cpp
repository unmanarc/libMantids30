#include "common_content.h"
#include <Mantids30/Core/Memory/streamable_null.h>
#include "common_content_chunked_subparser.h"
#include <Mantids30/DataFormats/JSON/streamable_json.h>

#include <memory>
#include <optional>
#include <stdexcept>

// TODO: CHECK THIS CLASS
using namespace Mantids30::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

Mantids30::Protocols::HTTP::Content::Content()
{
    m_subParserName.clear();
}

bool Mantids30::Protocols::HTTP::Content::isDefaultStreamableObj() const
{
    return m_usingInternalOutStream;
}

void Mantids30::Protocols::HTTP::Content::setSecurityMaxPostDataSize(const size_t &value)
{
    m_securityMaxPostDataSize = value;
}

Mantids30::Core::Memory::Streams::SubParser::ParseResult Mantids30::Protocols::HTTP::Content::parse()
{
    switch (m_currentMode)
    {
    case ProcessingMode::CHUNK_CLOSE:
    {
        // TODO: check if there is data, because may be new headers here...
        m_contentStreamableObject->writeEOF();
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
    }
    case ProcessingMode::CHUNK_SIZE:
    {
        std::optional<size_t> targetChunkSize;
        if ((targetChunkSize = parseHttpChunkSize()) != std::nullopt)
        {
            if (targetChunkSize > 0)
            {
                setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::SIZE);
                setParseDataTargetSize(*targetChunkSize);
                m_currentMode = ProcessingMode::CHUNK_DATA;
                return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
            }
            else
            {
                // Done... last chunk. Get the last \r\n here..
                setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DELIMITER);
                setParseDelimiter("\r\n");
                setParseDataTargetSize(64 * KB_MULT); // !64kb max (until fails, this may include new headers?).
                m_currentMode = ProcessingMode::CHUNK_CLOSE;
                return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
            }
        }
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::ERROR;
    }
    case ProcessingMode::CHUNK_DATA:
    {
        // Ok, continue
        setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::SIZE);
        setParseDataTargetSize(2); // for CRLF.
        m_currentMode = ProcessingMode::CHUNK_CRLF;
        // Proccess chunk into mem...
        // TODO: validate when outstream is filled up.
        getParsedBuffer()->appendTo(*m_contentStreamableObject);
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
    }
    case ProcessingMode::CHUNK_CRLF:
    {
        setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(1 * KB_MULT); // !1kb max (until fails).
        m_currentMode = ProcessingMode::CHUNK_SIZE;
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
    }
    case ProcessingMode::CONTENT_LENGTH:
    {
        // TODO: validate when outstream is filled up.
        getParsedBuffer()->appendTo(*m_contentStreamableObject);
        if (getUnparsedDataSize() > 0)
        {
#ifdef DEBUG
            printf("%p Content ProcessingMode::CONTENT_LENGTH - left to parse: %lu\n", this, getUnparsedDataSize());
#endif
            return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
        }
        else
        {
#ifdef DEBUG
            printf("%p Content ProcessingMode::CONTENT_LENGTH - EOS\n", this);
#endif
            // End of stream reached...
            m_contentStreamableObject->writeEOF();
            return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
        }
    }
    case ProcessingMode::CONNECTION_CLOSE:
    {
        // Content satisfied...
        if (getParsedBuffer()->size())
        {
            // Parsing data...
            // TODO: validate when outstream is filled up.
            getParsedBuffer()->appendTo(*m_contentStreamableObject);
            return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GET_MORE_DATA;
        }
        else
        {
            // No more data to parse, ending and processing it...
            m_contentStreamableObject->writeEOF();
            return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
        }
    }
    }
    return Mantids30::Core::Memory::Streams::SubParser::ParseResult::ERROR;
}

std::optional<uint32_t> Mantids30::Protocols::HTTP::Content::parseHttpChunkSize()
{
    const std::optional<uint32_t> parsed = getParsedBuffer()->toUInt32(16);

    if (!parsed)
    {
        return std::nullopt;
    }

    const uint32_t chunkSize = *parsed;

    // Max chunk size exceeded.
    if (chunkSize > m_securityMaxHttpChunkSize)
    {
        return std::nullopt;
    }

    // Already bad?
    if (m_currentContentLengthSize > m_securityMaxPostDataSize)
    {
        return std::nullopt;
    }

    // Remaining Available Size...
    const size_t remaining = m_securityMaxPostDataSize - m_currentContentLengthSize;

    if (chunkSize > remaining)
    {
        return std::nullopt;
    }

    m_currentContentLengthSize += chunkSize;

    return chunkSize;
}

Mantids30::Protocols::HTTP::Content::TransmissionMode Mantids30::Protocols::HTTP::Content::getTransmissionMode() const
{
    return m_transmitionMode;
}

std::shared_ptr<Mantids30::Protocols::HTTP::URLVars> Mantids30::Protocols::HTTP::Content::getUrlPostVars()
{
    if (m_containerType == ContainerType::URL && m_usingInternalOutStream)
    {
        return std::dynamic_pointer_cast<URLVars>(m_contentStreamableObject);
    }
    throw std::runtime_error("Invalid operation: getUrlPostVars should not be called when the content type is not URL.");
}

std::shared_ptr<Mantids30::DataFormats::JSON::StreamableJSON> Mantids30::Protocols::HTTP::Content::getJSONVars()
{
    if (m_containerType == ContainerType::JSON && m_usingInternalOutStream)
    {
        return std::dynamic_pointer_cast<Mantids30::DataFormats::JSON::StreamableJSON>(m_contentStreamableObject);
    }
    throw std::runtime_error("Invalid operation: getJSONVars should not be called when the content type is not JSON.");
}

std::shared_ptr<Mantids30::Protocols::MIME::MIME_Message> Mantids30::Protocols::HTTP::Content::getMultiPartVars()
{
    if (m_containerType == ContainerType::MIME && m_usingInternalOutStream)
    {
        return std::dynamic_pointer_cast<Mantids30::Protocols::MIME::MIME_Message>(m_contentStreamableObject);
    }
    throw std::runtime_error("Invalid operation: getMultiPartVars should not be called when the content type is not MIME.");
}

Mantids30::Protocols::HTTP::Content::ContainerType Mantids30::Protocols::HTTP::Content::getContainerType() const
{
    return m_containerType;
}

std::shared_ptr<Mantids30::Core::Memory::Abstract::Vars> Mantids30::Protocols::HTTP::Content::postVars()
{
    if (!m_usingInternalOutStream)
    {
        throw std::runtime_error("Error: not using internal output stream");
    }

    if (m_containerType == ContainerType::BIN || m_containerType == ContainerType::JSON)
    {
        // No vars... create an empty object.
        std::shared_ptr<Mantids30::Protocols::HTTP::URLVars> vars = Mantids30::Protocols::HTTP::URLVars::create(std::make_shared<Mantids30::Core::Memory::Streams::StreamableNull>());
        return vars;
    }

    std::shared_ptr<Mantids30::Core::Memory::Abstract::Vars> vars = std::dynamic_pointer_cast<Mantids30::Core::Memory::Abstract::Vars>(m_contentStreamableObject);
    if (!vars)
    {
        throw std::runtime_error("Error: outStream cannot be cast to Vars");
    }
    return vars;
}

void Mantids30::Protocols::HTTP::Content::setContainerType(const Mantids30::Protocols::HTTP::Content::ContainerType &value)
{
    m_containerType = value;
    if (m_usingInternalOutStream)
    {
        switch (m_containerType)
        {
        case ContainerType::MIME:
            m_contentStreamableObject = Mantids30::Protocols::MIME::MIME_Message::create();
            break;
        case ContainerType::URL:
            m_contentStreamableObject = URLVars::create();
            break;
        case ContainerType::JSON:
            m_contentStreamableObject = std::make_shared<Mantids30::DataFormats::JSON::StreamableJSON>();
            break;
        case ContainerType::BIN:
            m_contentStreamableObject = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
            break;
        }
    }
}

void Mantids30::Protocols::HTTP::Content::setSecurityMaxHttpChunkSize(const uint32_t &value)
{
    m_securityMaxHttpChunkSize = value;
}

bool Mantids30::Protocols::HTTP::Content::streamToUpstream()
{
    // Act as a client. Send data from here.
    switch (m_transmitionMode)
    {
    case TransmissionMode::CHUNKS:
    {
        ContentChunkedTransformer chunkedLiveTransformer(m_upStream);
        m_contentStreamableObject->streamTo(&chunkedLiveTransformer);
        return m_contentStreamableObject->writeStatus.succeed;
    }
    case TransmissionMode::CONTENT_LENGTH:
    case TransmissionMode::CONNECTION_CLOSE:
    {
        m_contentStreamableObject->streamTo(m_upStream);
        return m_contentStreamableObject->writeStatus.succeed;
    }
    }
    return true;
}

void Mantids30::Protocols::HTTP::Content::setTransmissionMode(const TransmissionMode &value)
{
    m_transmitionMode = value;

    switch (m_transmitionMode)
    {
    case TransmissionMode::CONNECTION_CLOSE:
    {
        // TODO: disable connection-close for client->server
        setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DIRECT);
        setParseDataTargetSize(m_securityMaxPostDataSize); // !1kb max (until fails).
        m_currentMode = ProcessingMode::CONNECTION_CLOSE;
    }
    break;
    case TransmissionMode::CHUNKS:
    {
        // TODO: disable chunk transmition for client->server
        setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(64); // 64 bytes max (until fails).
        m_currentMode = ProcessingMode::CHUNK_SIZE;
    }
    break;
    case TransmissionMode::CONTENT_LENGTH:
    {
        setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DIRECT);
        m_currentMode = ProcessingMode::CONTENT_LENGTH;
    }
    break;
    }
}

bool Mantids30::Protocols::HTTP::Content::setCurrentSize(const size_t &contentLengthSize)
{
    if (contentLengthSize > m_securityMaxPostDataSize)
    {
        // Can't receive this data...
        m_currentContentLengthSize = 0;
        setParseDataTargetSize(0);
        return false;
    }

    // The content length specified in the header
    m_currentContentLengthSize = contentLengthSize;
    setParseDataTargetSize(contentLengthSize);

    return true;
}

void Mantids30::Protocols::HTTP::Content::setMaxBinPostMemoryBeforeFS(const size_t &value)
{
    if (m_containerType == ContainerType::BIN && m_usingInternalOutStream)
    {
        std::dynamic_pointer_cast<Mantids30::Core::Memory::Containers::B_Chunks>(m_contentStreamableObject)->setMaxSizeInMemoryBeforeMovingToDisk(value);
    }
    else
    {
        throw std::runtime_error("This method is only meant for the internal container. Custom containers should be configured externally.");
    }
}

std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> Mantids30::Protocols::HTTP::Content::getStreamableObject()
{
    return m_contentStreamableObject;
}

void Mantids30::Protocols::HTTP::Content::setStreamableObject(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &outDataContainer)
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

size_t Mantids30::Protocols::HTTP::Content::getStreamSize()
{
    if (!m_contentStreamableObject)
    {
        return 0;
    }
    return m_contentStreamableObject->size();
}
