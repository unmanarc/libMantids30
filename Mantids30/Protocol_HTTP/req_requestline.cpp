#include "req_requestline.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include "streamdecoder_url.h"

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols::HTTP::Request;
using namespace Mantids30;

RequestLine::RequestLine()
{
    m_getVars = HTTP::URLVars::create();
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setSecurityMaxURLSize(128*KB_MULT); // 128K

    m_subParserName = "RequestLine";
}

bool RequestLine::streamToUpstream( Memory::Streams::WriteStatus & wrStat)
{
    Memory::Streams::WriteStatus cur;
    // Act as a client. Send data from here.
     if (!(cur+=m_upStream->writeString(m_requestMethod + " " + m_requestURI, wrStat )).succeed) return false;
    if (!m_getVars->isEmpty())
    {
        if (!(cur+=m_upStream->writeString("?",wrStat)).succeed) return false;
        if (!m_getVars->streamTo(m_upStream,wrStat)) return false;
    }
    if (!(cur+=m_upStream->writeString(" " + m_httpVersion.toString() + string("\r\n"), wrStat )).succeed) return false;
    return true;
}

Memory::Streams::SubParser::ParseStatus RequestLine::parse()
{
    std::string clientRequest = getParsedBuffer()->toString();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) return Memory::Streams::SubParser::PARSE_ERROR;

    m_requestMethod = boost::to_upper_copy(requestParts[0]);
    m_requestURI = requestParts[1];
    m_httpVersion.parse(requestParts.size()>2?requestParts[2]:"HTTP/1.0");

    parseURI();

    return Memory::Streams::SubParser::PARSE_GOTO_NEXT_SUBPARSER;
}

void RequestLine::parseURI()
{
    size_t found=m_requestURI.find("?");

    if (found!=std::string::npos)
    {
        // We have parameters..
        m_requestURIParameters = m_requestURI.c_str()+found+1;
        m_requestURI.resize(found);
        parseGETParameters();
    }
    else
    {
        // No parameters.
    }

    // Decode URI (maybe it's url encoded)...
    m_requestURI = Memory::Streams::Decoders::URL::decodeURLStr(m_requestURI);
}

void RequestLine::parseGETParameters()
{
    Memory::Streams::WriteStatus x;
    Memory::Containers::B_Chunks bc;

    bc.append(m_requestURIParameters.c_str(),m_requestURIParameters.size());
    bc.streamTo(m_getVars.get(),x);
}

std::string RequestLine::getRequestURIParameters() const
{
    return m_requestURIParameters;
}

HTTP::Version * RequestLine::getHTTPVersion()
{
    return &m_httpVersion;
}

std::shared_ptr<Memory::Abstract::Vars> RequestLine::urlVars()
{
    return m_getVars;
}

std::string RequestLine::getRequestMethod() const
{
    return m_requestMethod;
}

void RequestLine::setRequestMethod(const std::string &value)
{
    m_requestMethod = value;
}

std::string RequestLine::getURI() const
{
    return m_requestURI;
}

void RequestLine::setRequestURI(const std::string &value)
{
    m_requestURI = value;
}

void RequestLine::setSecurityMaxURLSize(size_t value)
{
    setParseDataTargetSize(value);
}

