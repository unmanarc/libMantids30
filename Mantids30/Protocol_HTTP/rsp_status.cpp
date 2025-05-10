#include "rsp_status.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30;

const HTTP::HTTPStatusCode HTTP::Status::m_responseStatusCodess[] = {
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
    {511,"Network Authentication Required"}
};


uint16_t HTTP::Status::getHTTPStatusCodeTranslation(const Status::Codes &code)
{
    if (code != HTTP::Status::S_999_NOT_SET)
    {
        return m_responseStatusCodess[code].code;
    }
    return 999;
}


HTTP::Status::Status()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setParseDataTargetSize(128);
    m_subParserName = "Status";

}

Memory::Streams::SubParser::ParseStatus HTTP::Status::parse()
{
    std::string clientRequest = getParsedBuffer()->toString();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) return Memory::Streams::SubParser::PARSE_ERROR;

    m_httpVersion.parse(requestParts[0]);
    m_responseStatusCodes = strtoul(requestParts[1].c_str(),nullptr,10);
    m_responseMessage = "";

    if (requestParts.size()>=3)
    {
        for (size_t i=2; i<requestParts.size(); i++)
        {
            if (i!=2) m_responseMessage+=" ";
            m_responseMessage+=requestParts[i];
        }
    }

    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
}

std::string HTTP::Status::getResponseMessage() const
{
    return m_responseMessage;
}

void HTTP::Status::setResponseMessage(const std::string &value)
{
    m_responseMessage = value;
}

bool HTTP::Status::streamToUpstream( Memory::Streams::WriteStatus & wrStat)
{
    // Act as a client. Send data from here.
    return m_upStream->writeString(  m_httpVersion.toString()
                                 +  " "
                                 +  std::to_string(m_responseStatusCodes)
                                 +  " "
                                 +  m_responseMessage + "\r\n",wrStat).succeed;
}

void HTTP::Status::setRetCodeValue(unsigned short value)
{
    m_responseStatusCodes = value;
}

void HTTP::Status::setRetCode(Status::Codes code)
{
    if (code != HTTP::Status::S_999_NOT_SET)
    {
        setRetCodeValue(m_responseStatusCodess[code].code);
        setResponseMessage(m_responseStatusCodess[code].responseMessage);
    }
}

unsigned short HTTP::Status::getRetCode() const
{
    return m_responseStatusCodes;
}

HTTP::Version *HTTP::Status::getHttpVersion()
{
    return &m_httpVersion;
}


