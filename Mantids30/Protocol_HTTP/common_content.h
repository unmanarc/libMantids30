#pragma once

#include "common_urlvars.h"
#include <Mantids30/Memory/streamable_json.h>

#include <Mantids30/Memory/b_base.h>
#include <Mantids30/Memory/subparser.h>
#include <Mantids30/Protocol_MIME/mime_message.h>
#include <memory>

namespace Mantids30::Network::Protocols::HTTP {

class Content : public Memory::Streams::SubParser
{
public:
    enum class ContainerType : uint8_t
    {
        BIN,
        MIME,
        URL,
        JSON
    };

    enum ProcessingMode : uint8_t
    {
        CHUNK_SIZE,
        CHUNK_DATA,
        CHUNK_CRLF,
        CHUNK_CLOSE,
        CONTENT_LENGTH,
        CONNECTION_CLOSE
    };

    enum class TransmissionMode : uint8_t
    {
        CHUNKS,
        CONTENT_LENGTH,
        CONNECTION_CLOSE
    };

    Content();
    ~Content() override = default;

    //////////////////////////////////////////////////////////////////
    //  ------------------ STREAMABLE OUTPUT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief stream Stream the output to upStream
     * @return true if written
     */
    bool streamToUpstream() override;
    /**
     * @brief isDefaultStreamableObj Get if the default streamable output is in use.
     * @return true if is it use, false if replaced by another one
     */
    bool isDefaultStreamableObj() const;
    /**
     * @brief writer Same of getStreamableObject
     * @return current streamable output
     */
    std::shared_ptr<Memory::Streams::StreamableObject> writer() { return m_contentStreamableObject; }
    /**
     * @brief getStreamableObject Get the current streamable output
     * @return current streamable output
     */
    std::shared_ptr<Memory::Streams::StreamableObject> getStreamableObject();
    /**
     * @brief setStreamableObj Set the streamable output (eg. a file?)
     * @param outDataContainer stream that will be used for the content trnasmission
     */
    void setStreamableObj(const std::shared_ptr<Memory::Streams::StreamableObject>& outDataContainer);
    /**
     * @brief getStreamSize Get stream full size ()
     * @return std::numeric_limits<size_t>::max() if size not defined, or >=0 if size defined.
     */
    size_t getStreamSize();
    /**
     * @brief postVars Get the post vars (read-only)... Useful for decoding received content
     * @return Variable
     */
    std::shared_ptr<Memory::Abstract::Vars> postVars();
    /**
     * @brief getMultiPartVars Get the MultiPart POST vars (read/write), call only when the ContainerType is MIME, otherwise, runtime error will be triggered
     * @return full MIME message
     */
    std::shared_ptr<MIME::MIME_Message> getMultiPartVars();
    /**
     * @brief getUrlPostVars Get URL formatted POST Vars (read/write), call only when the ContainerType is URL, otherwise, runtime error will be triggered
     * @return URL Var object
     */
    std::shared_ptr<URLVars> getUrlPostVars();
    /**
     * @brief getJSONVars Get JSON Stream
     * @return Unparsed JSON Stream
     */
    std::shared_ptr<Mantids30::Memory::Streams::StreamableJSON> getJSONVars();

    //////////////////////////////////////////////////////////////////
    // ------------------ TRANSMISSION AND CONTENT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief setTransmitionMode Set the transmission mode (chunked, content-lenght, connection-close)
     * @param value mode
     */
    void setTransmitionMode(const TransmissionMode &value);
    /**
     * @brief getTransmitionMode Get the transmission mode (chunked, content-lenght, connection-close)
     * @return mode
     */
    TransmissionMode getTransmitionMode() const;
    /**
     * @brief setCurrentSize Set content length data size (for receiving/decoding data).
     * @param contentLengthSize size.
     * @return true if limits are not exceeded.
     */
    bool setCurrentSize(const size_t &contentLengthSize);

    /**
     * @brief setContainerType Set Container Content Data Type (URL/MIME/BIN)
     * @param value type
     */
    void setContainerType(const ContainerType &value);
    /**
     * @brief getContainerType Get container content data type (URL/MIME/BIN)
     * @return type
     */
    ContainerType getContainerType() const;

    //////////////////////////////////////////////////
    // Security:
    /**
     * @brief Sets the maximum size of the internal memory buffer used to store HTTP POST data before the data is written to disk instead.
     * @param value The maximum size of the buffer, in bytes.
     */
    void setMaxBinPostMemoryBeforeFS(const size_t &value);
    void setSecurityMaxPostDataSize(const size_t &value);
    void setSecurityMaxHttpChunkSize(const uint32_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    std::shared_ptr<Memory::Streams::StreamableObject> m_contentStreamableObject = std::make_shared<Memory::Containers::B_Chunks>();
    bool m_usingInternalOutStream = true;

    std::optional<uint32_t> parseHttpChunkSize();

    // Parsing Optimization:
    TransmissionMode m_transmitionMode = TransmissionMode::CONNECTION_CLOSE;
    ProcessingMode m_currentMode = ProcessingMode::CONTENT_LENGTH;
    ContainerType m_containerType = ContainerType::BIN;

    // Security Parameters (for parsing):
    size_t m_securityMaxPostDataSize = 17 * MB_MULT; // 17Mb intermediate buffer (suitable for 16mb max chunk...).
    size_t m_currentContentLengthSize = 0;
    uint32_t m_securityMaxHttpChunkSize = 16 * MB_MULT; // 16mb.
};

} // namespace Mantids30::Network::Protocols::HTTP
