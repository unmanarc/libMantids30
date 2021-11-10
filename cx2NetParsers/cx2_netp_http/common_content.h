#ifndef HTTP_CONTENT_H
#define HTTP_CONTENT_H

#include "common_urlvars.h"

#include <cx2_mem_vars/substreamparser.h>
#include <cx2_mem_vars/b_base.h>
#include <cx2_netp_mime/mime_vars.h>

namespace CX2 { namespace Network { namespace HTTP { namespace Common {



enum eContent_TransmitionMode {
    TRANSMIT_MODE_CHUNKS,
    TRANSMIT_MODE_CONTENT_LENGTH,
    TRANSMIT_MODE_CONNECTION_CLOSE
};

enum eContent_ProcessingMode {
    PROCMODE_CHUNK_SIZE,
    PROCMODE_CHUNK_DATA,
    PROCMODE_CHUNK_CRLF,
    PROCMODE_CONTENT_LENGTH,
    PROCMODE_CONNECTION_CLOSE
};

enum eContent_DataType {
    CONTENT_TYPE_BIN,
    CONTENT_TYPE_MIME,
    CONTENT_TYPE_URL
};

class Content : public Memory::Streams::Parsing::SubParser
{
public:
    Content();
    ~Content() override;

    bool isDefaultStreamableOutput();

    void setContainerType(const eContent_DataType &value);

    void setTransmitionMode(const eContent_TransmitionMode &value);
    eContent_TransmitionMode getTransmitionMode() const;

    /**
     * @brief setContentLenSize Set content length data size.
     * @param contentLengthSize size.
     * @return true if limits are not exceeded.
     */
    bool setContentLenSize(const uint64_t &contentLengthSize);
    void useFilesystem(const std::string & fsFilePath);


    void preemptiveDestroyStreamableOuput();
    Memory::Streams::Streamable * getStreamableOuput();
    void setStreamableOutput(Memory::Streams::Streamable * outStream, bool deleteOutStream = false);
    /**
     * @brief getStreamSize Get stream full size ()
     * @return std::numeric_limits<uint64_t>::max() if size not defined, or >=0 if size defined.
     */
    uint64_t getStreamSize();

    //////////////////////////////////////////////////
    // Security:
    void setMaxPostSizeInMemBeforeGoingToFS(const uint64_t &value);
    void setSecurityMaxPostDataSize(const uint64_t &value);
    void setSecurityMaxHttpChunkSize(const uint32_t &value);

    bool stream(Memory::Streams::Status & wrStat) override;

    eContent_DataType getContainerType() const;

    Memory::Abstract::Vars * postVars();

    MIME::MIME_Vars * getMultiPartVars();
    URLVars * getUrlVars();

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    Memory::Streams::Streamable * outStream;
    bool deleteOutStream;

    uint32_t parseHttpChunkSize();

    // Parsing Optimization:
    eContent_TransmitionMode transmitionMode;
    eContent_ProcessingMode currentMode;
    eContent_DataType containerType;

    // Security Parameters (for parsing):
    uint64_t securityMaxPostDataSize;
    uint64_t currentContentLengthSize;
    uint32_t securityMaxHttpChunkSize;

    Memory::Containers::B_Chunks binDataContainer;

    MIME::MIME_Vars multiPartVars;
    URLVars urlVars;
};

}}}}

#endif // HTTP_CONTENT_H
