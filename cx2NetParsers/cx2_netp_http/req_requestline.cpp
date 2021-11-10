#include "req_requestline.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include "streamdecoder_url.h"

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2::Network::HTTP::Request;
using namespace CX2;

RequestLine::RequestLine()
{
    requestMethod = "GET"; // Default Method.

    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setSecurityMaxURLSize(128*KB_MULT); // 128K
}

bool RequestLine::stream(Memory::Streams::Status & wrStat)
{
    Memory::Streams::Status cur;
    // Act as a client. Send data from here.
    if (!(cur+=upStream->writeString(requestMethod + " " + requestURI, wrStat )).succeed) return false;
    if (!getVars.isEmpty())
    {
        if (!(cur+=upStream->writeString("?",wrStat)).succeed) return false;
        if (!getVars.streamTo(upStream,wrStat)) return false;
    }
    if (!(cur+=upStream->writeString(" " + httpVersion.getHTTPVersionString() + string("\r\n"), wrStat )).succeed) return false;
    return true;
}

Memory::Streams::Parsing::ParseStatus RequestLine::parse()
{
    std::string clientRequest = getParsedData()->toString();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) return Memory::Streams::Parsing::PARSE_STAT_ERROR;

    requestMethod = boost::to_upper_copy(requestParts[0]);
    requestURI = requestParts[1];
    httpVersion.parseVersion(requestParts.size()>2?requestParts[2]:"HTTP/1.0");

    parseURI();

    return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

void RequestLine::parseURI()
{
    size_t found=requestURI.find("?");

    if (found!=std::string::npos)
    {
        // We have parameters..
        requestURIParameters = requestURI.c_str()+found+1;
        requestURI.resize(found);
        parseGETParameters();
    }
    else
    {
        // No parameters.
    }

    // Decode URI (maybe it's url encoded)...
    requestURI = Memory::Streams::Decoders::URL::decodeURLStr(requestURI);
}

void RequestLine::parseGETParameters()
{
    Memory::Streams::Status x;
    Memory::Containers::B_Chunks bc;
    bc.append(requestURIParameters.c_str(),requestURIParameters.size());
    bc.streamTo(&getVars,x);
}

std::string RequestLine::getRequestURIParameters() const
{
    return requestURIParameters;
}

Common::Version * RequestLine::getHTTPVersion()
{
    return &httpVersion;
}

Memory::Abstract::Vars *RequestLine::getVarsPTR()
{
    return &getVars;
}

std::string RequestLine::getRequestMethod() const
{
    return requestMethod;
}

void RequestLine::setRequestMethod(const std::string &value)
{
    requestMethod = value;
}

std::string RequestLine::getURI() const
{
    return requestURI;
}

void RequestLine::setRequestURI(const std::string &value)
{
    requestURI = value;
}

void RequestLine::setSecurityMaxURLSize(size_t value)
{
    setParseDataTargetSize(value);
}

