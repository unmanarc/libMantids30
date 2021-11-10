#ifndef HTTP_CODE_RESPONSE_H
#define HTTP_CODE_RESPONSE_H

#include <cx2_mem_vars/substreamparser.h>
#include "common_version.h"
#include "rsp_statuscodes.h"

namespace CX2 { namespace Network { namespace HTTP { namespace Response {

struct sHTTP_StatusCode
{
    sHTTP_StatusCode(const uint16_t &code,const char *responseMessage)
    {
        this->code = code;
        this->responseMessage = responseMessage;
    }
    uint16_t code;
    std::string responseMessage;
};

class Status : public Memory::Streams::Parsing::SubParser
{
public:
    Status();

    static uint16_t getHTTPStatusCodeTranslation(const StatusCode & code);

    /**
     * @brief getHttpVersion - Get HTTP Version Object
     * @return Version Object
     */
    Common::Version * getHttpVersion();
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
    void setRetCode(StatusCode code);
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
    Common::Version httpVersion;
    unsigned short responseCode;
    std::string responseMessage;

    static const sHTTP_StatusCode responseCodes[];


};

}}}}

#endif // HTTP_CODE_RESPONSE_H
