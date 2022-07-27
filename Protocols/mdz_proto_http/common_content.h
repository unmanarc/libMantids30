#ifndef HTTP_CONTENT_H
#define HTTP_CONTENT_H

#include "common_urlvars.h"

#include <mdz_mem_vars/subparser.h>
#include <mdz_mem_vars/b_base.h>
#include <mdz_proto_mime/mime_message.h>

namespace Mantids { namespace Protocols { namespace HTTP { namespace Common {

class Content : public Memory::Streams::SubParser
{
public:
    enum eDataType {
        CONTENT_TYPE_BIN,
        CONTENT_TYPE_MIME,
        CONTENT_TYPE_URL
    };

    enum eProcessingMode {
        PROCMODE_CHUNK_SIZE,
        PROCMODE_CHUNK_DATA,
        PROCMODE_CHUNK_CRLF,
        PROCMODE_CONTENT_LENGTH,
        PROCMODE_CONNECTION_CLOSE
    };

    enum eTransmitionMode {
        TRANSMIT_MODE_CHUNKS,
        TRANSMIT_MODE_CONTENT_LENGTH,
        TRANSMIT_MODE_CONNECTION_CLOSE
    };

    Content();
    ~Content() override;


    //////////////////////////////////////////////////////////////////
    //  ------------------ STREAMABLE OUTPUT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief stream Stream the output to upStream
     * @param wrStat write status
     * @return true if written
     */
    bool stream(Memory::Streams::StreamableObject::Status & wrStat) override;
    /**
     * @brief isDefaultStreamableObj Get if the default streamable output is in use.
     * @return true if is it use, false if replaced by another one
     */
    bool isDefaultStreamableObj();
    /**
     * @brief preemptiveDestroyStreamableObj Destroy the current streamable output (if marked for deletion) and use the internal one.
     */
    void preemptiveDestroyStreamableObj();
    /**
     * @brief writer Same of getStreamableObj
     * @return current streamable output
     */
    Memory::Streams::StreamableObject * writer() { return outStream; }
    /**
     * @brief getStreamableObj Get the current streamable output
     * @return current streamable output
     */
    Memory::Streams::StreamableObject * getStreamableObj();
    /**
     * @brief setStreamableObj Set the streamable output (eg. a file?)
     * @param outStream stream that will be used for the content trnasmission
     * @param deleteOutStream delete the stream after deleting this class
     */
    void setStreamableObj(Memory::Streams::StreamableObject * outStream, bool deleteOutStream = false);
    /**
     * @brief getStreamSize Get stream full size ()
     * @return std::numeric_limits<uint64_t>::max() if size not defined, or >=0 if size defined.
     */
    uint64_t getStreamSize();
    /**
     * @brief postVars Get the post vars (read-only)... Useful for decoding received content
     * @return Variable
     */
    Memory::Abstract::Vars * postVars();
    /**
     * @brief getMultiPartVars Get the MultiPart POST vars (read/write), call only when the ContainerType is CONTENT_TYPE_MIME, otherwise, runtime error will be triggered
     * @return full MIME message
     */
    MIME::MIME_Message * getMultiPartVars();
    /**
     * @brief getUrlPostVars Get URL formatted POST Vars (read/write), call only when the ContainerType is CONTENT_TYPE_URL, otherwise, runtime error will be triggered
     * @return URL Var object
     */
    URLVars * getUrlPostVars();

    //////////////////////////////////////////////////////////////////
    // ------------------ TRANSMISSION AND CONTENT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief setTransmitionMode Set the transmission mode (chunked, content-lenght, connection-close)
     * @param value mode
     */
    void setTransmitionMode(const eTransmitionMode &value);
    /**
     * @brief getTransmitionMode Get the transmission mode (chunked, content-lenght, connection-close)
     * @return mode
     */
    eTransmitionMode getTransmitionMode() const;
    /**
     * @brief setContentLenSize Set content length data size (for receiving/decoding data).
     * @param contentLengthSize size.
     * @return true if limits are not exceeded.
     */
    bool setContentLenSize(const uint64_t &contentLengthSize);
    /**
     * @brief useFilesystem Set filesystem path when variables exceed limits
     * @param fsFilePath file path
     *
    void useFilesystem(const std::string & fsFilePath);*/
    /**
     * @brief setContainerType Set Container Content Data Type (URL/MIME/BIN)
     * @param value type
     */
    void setContainerType(const eDataType &value);
    /**
     * @brief getContainerType Get container content data type (URL/MIME/BIN)
     * @return type
     */
    eDataType getContainerType() const;


    //////////////////////////////////////////////////
    // Security:
    void setMaxPostSizeInMemBeforeGoingToFS(const uint64_t &value);
    void setSecurityMaxPostDataSize(const uint64_t &value);
    void setSecurityMaxHttpChunkSize(const uint32_t &value);




protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    Memory::Streams::StreamableObject * outStream;
    bool deleteOutStream;

    uint32_t parseHttpChunkSize();

    // Parsing Optimization:
    eTransmitionMode transmitionMode;
    eProcessingMode currentMode;
    eDataType containerType;

    // Security Parameters (for parsing):
    uint64_t securityMaxPostDataSize;
    uint64_t currentContentLengthSize;
    uint32_t securityMaxHttpChunkSize;

    Memory::Containers::B_Chunks binDataContainer;

    MIME::MIME_Message multiPartVars;
    URLVars urlPostVars;
};

}}}}

#endif // HTTP_CONTENT_H
