#include "rsp_status.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2::Network::HTTP::Response;
using namespace CX2;

const sHTTP_StatusCode Status::responseCodes[] = {
{100,"Continue"},
{101,"Switching Protocol"},
{200,"OK"},
{201,"Created"},
{202,"Accepted"},
{203,"Non-Authoritative Information"},
{204,"No Content"},
{205,"Reset Content"},
{206,"Partial Content"},
{300,"Multiple Choices"},
{301,"Moved Permanently"},
{302,"Found"},
{303,"See Other"},
{304,"Not Modified"},
{307,"Temporary Redirect"},
{308,"Permanent Redirect"},
{400,"Bad Request"},
{401,"Unauthorized"},
{403,"Forbidden"},
{404,"Not Found"},
{405,"Method Not Allowed"},
{406,"Not Acceptable"},
{407,"Proxy Authentication Required"},
{408,"Request Timeout"},
{409,"Conflict"},
{410,"Gone"},
{411,"Length Required"},
{412,"Precondition Failed"},
{413,"Payload Too Large"},
{414,"URI Too Long"},
{415,"Unsupported Media Type"},
{416,"Range Not Satisfiable"},
{417,"Expectation Failed"},
{426,"Upgrade Required"},
{428,"Precondition Required"},
{429,"Too Many Requests"},
{431,"Request Header Fields Too Large"},
{451,"Unavailable For Legal Reasons"},
{500,"Internal Server Error"},
{501,"Not Implemented"},
{502,"Bad Gateway"},
{503,"Service Unavailable"},
{504,"Gateway Timeout"},
{505,"HTTP Version Not Supported"},
{511,"Network Authentication Required"}};


Status::Status()
{
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setParseDataTargetSize(128);
}

uint16_t Status::getHTTPStatusCodeTranslation(const StatusCode &code)
{
    if (code != Response::StatusCode::S_999_NOT_SET)
    {
        return responseCodes[code].code;
    }
    return 999;
}

Memory::Streams::Parsing::ParseStatus Status::parse()
{
    std::string clientRequest = getParsedData()->toString();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) return Memory::Streams::Parsing::PARSE_STAT_ERROR;

    httpVersion.parseVersion(requestParts[0]);
    responseCode = strtoul(requestParts[1].c_str(),nullptr,10);
    responseMessage = "";

    if (requestParts.size()>=3)
    {
        for (size_t i=2; i<requestParts.size(); i++)
        {
            if (i!=2) responseMessage+=" ";
            responseMessage+=requestParts[i];
        }
    }

    return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

std::string Status::getResponseMessage() const
{
    return responseMessage;
}

void Status::setResponseMessage(const std::string &value)
{
    responseMessage = value;
}

bool Status::stream(Memory::Streams::Status & wrStat)
{
    // Act as a client. Send data from here.
    return upStream->writeString(  httpVersion.getHTTPVersionString()
                                 +  " "
                                 +  std::to_string(responseCode)
                                 +  " "
                                 +  responseMessage + "\r\n",wrStat).succeed;
}

void Status::setRetCodeValue(unsigned short value)
{
    responseCode = value;
}

void Status::setRetCode(StatusCode code)
{
    if (code != Response::StatusCode::S_999_NOT_SET)
    {
        setRetCodeValue(responseCodes[code].code);
        setResponseMessage(responseCodes[code].responseMessage);
    }
}

unsigned short Status::getRetCode() const
{
    return responseCode;
}

Common::Version *Status::getHttpVersion()
{
    return &httpVersion;
}


