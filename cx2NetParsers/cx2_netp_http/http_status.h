#ifndef HTTP_CODE_RESPONSE_H
#define HTTP_CODE_RESPONSE_H

#include <cx2_mem_vars/substreamparser.h>
#include "http_version.h"
#include "http_retcodes.h"

namespace CX2 { namespace Network { namespace HTTP {


struct sHTTP_ResponseCode
{
    sHTTP_ResponseCode(const uint16_t &code,const char *responseMessage)
    {
        this->code = code;
        this->responseMessage = responseMessage;
    }
    uint16_t code;
    std::string responseMessage;
};

class HTTP_Status : public Memory::Streams::Parsing::SubParser
{
public:
    HTTP_Status();

    static uint16_t getHTTPRetCodeValue(const eHTTP_RetCode & code);

    /**
     * @brief getHttpVersion - Get HTTP Version Object
     * @return Version Object
     */
    HTTP_Version * getHttpVersion();
    /**
     * @brief getResponseCode - Get HTTP Response Code (Ex. 404=Not found)
     * @return response code number
     */
    unsigned short getRetCode() const;
    /**
     * @brief setResponseCode - Set HTTP Response Code (Ex. 404=Not found)
     * @param value response code number
     */
    void setRetCodeValue(unsigned short value);
    /**
     * @brief setResponseCode2 Set response code and message from a fixed list.
     */
    void setRetCode(eHTTP_RetCode code);
    /**
     * @brief getResponseMessage - Get HTTP Response Code Message (Ex. Not found)
     * @return response code message
     */
    std::string getResponseMessage() const;
    /**
     * @brief setResponseMessage - Set HTTP Response Code Message (Ex. Not found)
     * @param value response code message
     */
    void setResponseMessage(const std::string &value);

    bool stream(Memory::Streams::Status & wrStat) override;
protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    HTTP_Version httpVersion;
    unsigned short responseCode;
    std::string responseMessage;

    static const sHTTP_ResponseCode responseCodes[];


};

}}}

#endif // HTTP_CODE_RESPONSE_H
