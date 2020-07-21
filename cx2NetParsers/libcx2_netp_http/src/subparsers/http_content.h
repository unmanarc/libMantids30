#ifndef HTTP_CONTENT_H
#define HTTP_CONTENT_H

#include <cx2_mem_streamparser/substreamparser.h>
#include <cx2_mem_containers/b_base.h>

#include <cx2_netp_urlvars/url_vars.h>
#include <cx2_netp_mime/multipart_vars.h>

namespace CX2 { namespace Network { namespace Parsers {


enum HTTP_Content_Transmition_Mode {
    HTTP_CONTENT_TRANSMODE_CHUNKS,
    HTTP_CONTENT_TRANSMODE_CONTENT_LENGTH,
    HTTP_CONTENT_TRANSMODE_CONNECTION_CLOSE
};

enum HTTP_ContentData_CurrentMode {
    HTTP_CONTENTDATA_CURRMODE_CHUNK_SIZE,
    HTTP_CONTENTDATA_CURRMODE_CHUNK_DATA,
    HTTP_CONTENTDATA_CURRMODE_CHUNK_CRLF,
    HTTP_CONTENTDATA_CURRMODE_CONTENT_LENGTH,
    HTTP_CONTENTDATA_CURRMODE_CONNECTION_CLOSE
};

enum HTTP_ContainerType {
    HTTP_CONTAINERTYPE_BIN,
    HTTP_CONTAINERTYPE_MIME,
    HTTP_CONTAINERTYPE_URL
};



class HTTP_Content : public Memory::Streams::Parsing::SubParser
{
public:
    HTTP_Content();
    ~HTTP_Content() override;

    bool isDefaultStreamableOutput();

    void setContainerType(const HTTP_ContainerType &value);

    void setTransmitionMode(const HTTP_Content_Transmition_Mode &value);
    HTTP_Content_Transmition_Mode getTransmitionMode() const;

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

    HTTP_ContainerType getContainerType() const;

    Memory::Vars::Vars * postVars();

    MultiPart_Vars * getMultiPartVars();
    URL_Vars * getUrlVars();

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    Memory::Streams::Streamable * outStream;
    bool deleteOutStream;

    uint32_t parseHttpChunkSize();

    // Parsing Optimization:
    HTTP_Content_Transmition_Mode transmitionMode;
    HTTP_ContentData_CurrentMode currentMode;
    HTTP_ContainerType containerType;

    // Security Parameters (for parsing):
    uint64_t securityMaxPostDataSize;
    uint64_t currentContentLengthSize;
    uint32_t securityMaxHttpChunkSize;

    Memory::Containers::B_Chunks binDataContainer;

    MultiPart_Vars multiPartVars;
    URL_Vars urlVars;
};

}}}

#endif // HTTP_CONTENT_H
