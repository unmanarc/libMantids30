#include "req_requestline.h"

#include "Mantids30/Core/Memory/streamable_string.h"
#include "streamdecoder_url.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>

#include <string>
#include <vector>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Protocol;
using namespace Mantids30::Protocol::HTTP::Request;
using namespace Mantids30;

RequestLine::RequestLine()
{
    m_getVars = Mantids30::Protocol::HTTP::URLVars::create();
    setParseStrategy(Mantids30::Core::Memory::Streams::SubParser::ParseStrategy::DELIMITER);
    setParseDelimiter("\r\n");
    setSecurityMaxURLSize(128 * KB_MULT); // 128K

    m_subParserName = "RequestLine";
}

bool RequestLine::streamToUpstream()
{
    // Act as a client. Send data from here.
    std::string buffer = m_requestMethod + " " + m_requestURI;

    if (!m_getVars->isEmpty())
    {
        buffer += m_requestURI.find('?') == std::string::npos ? "?" : "&";

        Mantids30::Core::Memory::Streams::StreamableString getVarsStr;
        m_getVars->streamTo(&getVarsStr);
        buffer += getVarsStr.getValue();
    }

    buffer += " " + m_httpVersion.toString() + "\r\n";

    m_upStream->writeString(buffer);
    return m_upStream->writeStatus.succeed;
}

Mantids30::Core::Memory::Streams::SubParser::ParseResult RequestLine::parse()
{
    std::string clientRequest = getParsedBuffer()->toStringEx();

    vector<string> requestParts;
    split(requestParts, clientRequest, is_any_of("\t "), token_compress_on);

    // We need almost 2 parameters.
    if (isStreamEnded() || requestParts.size() < 2)
    {
        return Mantids30::Core::Memory::Streams::SubParser::ParseResult::PARSE_ERROR;
    }

    if (requestParts.size() != 3)
    {
        return ParseResult::PARSE_ERROR;
    }
    static const boost::regex versionPattern(R"(HTTP/[0-9]\.[0-9])");
    if (!boost::regex_match(requestParts[2], versionPattern))
    {
        return ParseResult::PARSE_ERROR;
    }

    m_requestMethod = boost::to_upper_copy(requestParts[0]);
    m_requestURI = requestParts[1];
    m_httpVersion.parse(requestParts.size() > 2 ? requestParts[2] : "HTTP/1.0");

    parseURI();

    return Mantids30::Core::Memory::Streams::SubParser::ParseResult::GOTO_NEXT_SUBPARSER;
}

void RequestLine::parseURI()
{
    size_t found = m_requestURI.find('?');

    if (found != std::string::npos)
    {
        // We have parameters..
        m_requestGETVarsRawString = m_requestURI.c_str() + found + 1;
        m_requestURI.resize(found);
        parseGETVars();
    }
    else
    {
        // No parameters.
    }

    // Decode URI (maybe it's url encoded)...
    m_requestURI = Mantids30::Protocol::HTTP::Decoders::URL::decodeURLStr(m_requestURI);
}

void RequestLine::parseGETVars()
{
    m_getVars->writeFullStreamWithEOF(m_requestGETVarsRawString.c_str(), m_requestGETVarsRawString.size());
}

std::string RequestLine::getRequestGETVarsRawString() const
{
    return m_requestGETVarsRawString;
}

bool RequestLine::fromJSON(const boost::json::value &json)
{
    if (!json.is_object())
    {
        return false;
    }

    setHTTPMethod(Mantids30::DataFormat::JSON::ASSTRING(json, "method", ""));
    setRequestURI(Mantids30::DataFormat::JSON::ASSTRING(json, "uri", ""));
    m_httpVersion.fromJSON(Mantids30::DataFormat::JSON::MEMBER_OR_NULL(json, "httpVersion"));
    m_getVars->fromJSON(Mantids30::DataFormat::JSON::MEMBER_OR_NULL(json, "getVars"));

    return true;
}

boost::json::object RequestLine::toJSON() const
{
    boost::json::object json;
    json["method"] = getHTTPMethod();
    json["uri"] = getURI();
    json["httpVersion"] = m_httpVersion.toJSON();
    json["getVars"] = m_getVars->toJSON();
    return json;
}

string RequestLine::toString() const
{
    if (m_requestGETVarsRawString.empty())
    {
        return getHTTPMethod() + " " + getURI() + " " + m_httpVersion.toString();
    }
    else
    {
        return getHTTPMethod() + " " + getURI() + "?" + m_requestGETVarsRawString + " " + m_httpVersion.toString();
    }
}

Mantids30::Protocol::HTTP::Version *RequestLine::getHTTPVersion()
{
    return &m_httpVersion;
}

std::shared_ptr<Mantids30::Core::Memory::Abstract::Vars> RequestLine::urlVars()
{
    return m_getVars;
}

std::string RequestLine::getHTTPMethod() const
{
    return m_requestMethod;
}

void RequestLine::setHTTPMethod(std::string_view value)
{
    m_requestMethod = value;
}

std::string RequestLine::getURI() const
{
    return m_requestURI;
}

void RequestLine::setRequestURI(std::string_view value)
{
    m_requestURI = value;
}

void RequestLine::setSecurityMaxURLSize(size_t value)
{
    setParseDataTargetSize(value);
}
