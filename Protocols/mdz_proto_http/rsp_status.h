#ifndef HTTP_CODE_RESPONSE_H
#define HTTP_CODE_RESPONSE_H

#include <mdz_mem_vars/subparser.h>
#include "common_version.h"

namespace Mantids { namespace Protocols { namespace HTTP {

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

class Status : public Memory::Streams::SubParser
{
public:
    enum eRetCode {
        S_100_CONTINUE = 0,
        S_101_SWITCHING_PROTOCOL = 1,
        S_200_OK = 2,
        S_201_CREATED = 3,
        S_202_ACCEPTED = 4,
        S_203_NON_AUTHORITATIVE_INFORMATION = 5,
        S_204_NO_CONTENT = 6,
        S_205_RESET_CONTENT = 7,
        S_206_PARTIAL_CONTENT = 8,
        S_300_MULTIPLE_CHOICES = 9,
        S_301_MOVED_PERMANENTLY = 10,
        S_302_FOUND = 11,
        S_303_SEE_OTHER = 12,
        S_304_NOT_MODIFIED = 13,
        S_307_TEMPORARY_REDIRECT = 14,
        S_308_PERMANENT_REDIRECT = 15,
        S_400_BAD_REQUEST = 16,
        S_401_UNAUTHORIZED = 17,
        S_403_FORBIDDEN = 18,
        S_404_NOT_FOUND = 19,
        S_405_METHOD_NOT_ALLOWED = 20,
        S_406_NOT_ACCEPTABLE = 21,
        S_407_PROXY_AUTHENTICATION_REQUIRED = 22,
        S_408_REQUEST_TIMEOUT = 23,
        S_409_CONFLICT = 24,
        S_410_GONE = 25,
        S_411_LENGTH_REQUIRED = 26,
        S_412_PRECONDITION_FAILED = 27,
        S_413_PAYLOAD_TOO_LARGE = 28,
        S_414_URI_TOO_LONG = 29,
        S_415_UNSUPPORTED_MEDIA_TYPE = 30,
        S_416_RANGE_NOT_SATISFIABLE = 31,
        S_417_EXPECTATION_FAILED = 32,
        S_426_UPGRADE_REQUIRED = 33,
        S_428_PRECONDITION_REQUIRED = 34,
        S_429_TOO_MANY_REQUESTS = 35,
        S_431_REQUEST_HEADER_FIELDS_TOO_LARGE = 36,
        S_451_UNAVAILABLE_FOR_LEGAL_REASONS = 37,
        S_500_INTERNAL_SERVER_ERROR = 38,
        S_501_NOT_IMPLEMENTED = 39,
        S_502_BAD_GATEWAY = 40,
        S_503_SERVICE_UNAVAILABLE = 41,
        S_504_GATEWAY_TIMEOUT = 42,
        S_505_HTTP_VERSION_NOT_SUPPORTED = 43,
        S_511_NETWORK_AUTHENTICATION_REQUIRED = 44,
        S_999_NOT_SET = 1000
    };

    Status();

    static uint16_t getHTTPStatusCodeTranslation(const eRetCode & code);

    /**
     * @brief getHttpVersion - Get HTTP Version Object
     * @return Version Object
     */
    Common::Version * getHttpVersion();
    /**
     * @brief getResponseRetCode - Get HTTP Response Code (Ex. 404=Not found)
     * @return response code number
     */
    unsigned short getRetCode() const;
    /**
     * @brief setResponseRetCode - Set HTTP Response Code (Ex. 404=Not found)
     * @param value response code number
     */
    void setRetCodeValue(unsigned short value);
    /**
     * @brief setResponseRetCode2 Set response code and message from a fixed list.
     */
    void setRetCode(eRetCode code);
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

    bool stream(Memory::Streams::StreamableObject::Status & wrStat) override;
protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    Common::Version httpVersion;
    unsigned short responseRetCode;
    std::string responseMessage;

    static const sHTTP_StatusCode responseRetCodes[];


};
}}}

#endif // HTTP_CODE_RESPONSE_H
