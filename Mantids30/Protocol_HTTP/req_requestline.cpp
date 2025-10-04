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

bool RequestLine::streamToUpstream()
{
    // Act as a client. Send data from here.
    m_upStream->writeString(m_requestMethod + " " + m_requestURI );
    if (!m_upStream->writeStatus.succeed)
        return false;

    if (!m_getVars->isEmpty())
    {
        m_upStream->writeString("?");

        if (!m_upStream->writeStatus.succeed)
            return false;

        if (!m_getVars->streamTo(m_upStream))
            return false;
    }

    m_upStream->writeString(" " + m_httpVersion.toString() + string("\r\n") );

    if (!m_upStream->writeStatus.succeed)
        return false;

    return m_upStream->writeStatus.succeed;
}

Memory::Streams::SubParser::ParseStatus RequestLine::parse()
{
    std::string clientRequest = getParsedBuffer()->toStringEx();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) 
        return Memory::Streams::SubParser::PARSE_ERROR;

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
        m_requestGETVarsRawString = m_requestURI.c_str()+found+1;
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
    Memory::Containers::B_Chunks bc;

    bc.append(m_requestGETVarsRawString.c_str(),m_requestGETVarsRawString.size());
    bc.streamTo(m_getVars.get());
}

std::string RequestLine::getRequestGETVarsRawString() const
{
    return m_requestGETVarsRawString;
}

bool RequestLine::fromJSON(const Json::Value &json)
{
    if (!json.isObject())
        return false;

    setRequestMethod(JSON_ASSTRING(json, "method", ""));
    setRequestURI(JSON_ASSTRING(json, "uri", ""));
    m_httpVersion.fromJSON(json["httpVersion"]);
    m_getVars->fromJSON(json["getVars"]);

    return true;
}

json RequestLine::toJSON() const
{
    Json::Value json;
    json["method"] = getRequestMethod();
    json["uri"] = getURI();
    json["httpVersion"] = m_httpVersion.toJSON();
    json["getVars"] = m_getVars->toJSON();
    return json;
}

string RequestLine::toString() const
{
    if ( m_requestGETVarsRawString.empty() )
        return getRequestMethod() + " " + getURI() + " " + m_httpVersion.toString();
    else
        return getRequestMethod() + " " + getURI() + "?" + m_requestGETVarsRawString + " " + m_httpVersion.toString();
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

