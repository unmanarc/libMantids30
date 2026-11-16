#pragma once

#include <Mantids30/Core/Memory/streamable_string.h>
#include <Mantids30/Protocol/HTTP/httpv1_client.h>
#include <Mantids30/Protocol/HTTP/httpv1_server.h>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace HTTPParserTests {

using namespace Mantids30::Protocol::HTTP;
using Mantids30::Core::Memory::Streams::Parser;
using Mantids30::Core::Memory::Streams::StreamableString;

inline void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

struct RawCase
{
    std::string name;
    std::function<void(const std::vector<size_t> &)> run;
    bool allSchedules = true;
};

struct FeedResult
{
    size_t consumed = 0;
    bool rejected = false;
    bool finishedBeforeEOF = false;
};

// A zero fragment size means the entire remaining input. Every call must make
// progress; a short write can leave a subsequent HTTP message in the input.
inline FeedResult feed(Parser &parser, const std::string &raw, const std::vector<size_t> &fragments, bool eof = true)
{
    require(!fragments.empty(), "Fragment schedule must not be empty");
    FeedResult result;
    size_t step = 0;
    while (result.consumed < raw.size() && !parser.writeStatus.finished)
    {
        const size_t fragment = fragments[step++ % fragments.size()];
        const size_t count = std::min(fragment ? fragment : raw.size(), raw.size() - result.consumed);
        const auto written = parser.write(raw.data() + result.consumed, count);
        if (!written)
        {
            result.rejected = true;
            return result;
        }
        require(*written > 0 && *written <= count, "Parser returned an invalid consumed-byte count");
        result.consumed += *written;
    }
    result.finishedBeforeEOF = parser.writeStatus.finished;
    if (eof && !parser.writeStatus.finished)
    {
        result.rejected = !parser.writeEOF();
    }
    return result;
}

inline void completed(Parser &parser, const FeedResult &result, size_t expectedBytes)
{
    require(!result.rejected, "Unexpected parser rejection");
    require(parser.getParseResult() == Parser::ParseResult::SUCCEED, "Unexpected parser error status");
    require(parser.writeStatus.finished, "Parser did not complete the message");
    require(result.consumed == expectedBytes, "Wrong message boundary: expected " + std::to_string(expectedBytes)
                + " consumed bytes, got " + std::to_string(result.consumed));
}

// Keep examining prefixes after a failure so one missing check cannot hide
// truncated headers, chunk data, chunk CRLF or trailers later in the message.
inline void everyPrefix(const std::string &raw, const std::function<void(const std::string &)> &check)
{
    size_t failures = 0;
    std::string firstFailure;
    for (size_t length = 0; length < raw.size(); ++length)
    {
        try
        {
            check(raw.substr(0, length));
        }
        catch (const std::exception &error)
        {
            if (failures++ == 0)
            {
                firstFailure = "first at prefix " + std::to_string(length) + ": " + error.what();
            }
        }
    }
    require(failures == 0, std::to_string(failures) + "/" + std::to_string(raw.size())
                + " truncated prefixes failed; " + firstFailure);
}

class FailingSink : public Mantids30::Core::Memory::Streams::StreamableObject
{
public:
    explicit FailingSink(bool failOnEOF) : m_failOnEOF(failOnEOF) {}
    std::optional<size_t> write(const void *, const size_t &count) override
    {
        return (count == 0) == m_failOnEOF ? std::nullopt : std::optional<size_t>(count);
    }

private:
    bool m_failOnEOF;
};

class Client : public HTTPv1_Client
{
public:
    Client() : HTTPv1_Client(std::make_shared<StreamableString>()) {}
    size_t headerCalls = 0;
    bool acceptHeaders = true;

protected:
    bool onResponseHeaders() override
    {
        ++headerCalls;
        return acceptHeaders;
    }
};

class Server : public HTTPv1_Server
{
public:
    using HTTPv1_Server::setConnectionContinue;

    Server() : HTTPv1_Server(std::make_shared<StreamableString>())
    {
        setConnectionContinue(false);
        serverResponse.includeDate = false;
    }

    std::string events;
    bool acceptURI = true;
    bool acceptHeaders = true;
    size_t contentCalls = 0;
    std::function<void(Server &)> inspectContent;

    std::string response() const { return m_streamableObject->toString(); }

protected:
    bool onHTTPClientURIReceived() override
    {
        events += 'U';
        return acceptURI;
    }
    bool onHTTPClientHeadersReceived() override
    {
        events += 'H';
        return acceptHeaders;
    }
    Status::Code onHTTPClientContentReceived() override
    {
        events += 'C';
        ++contentCalls;
        if (inspectContent)
        {
            inspectContent(*this);
        }
        return Status::Code::S_200_OK;
    }
};

inline std::string request(const std::string &body, const std::string &type = "application/octet-stream")
{
    return "POST /upload HTTP/1.1\r\nHost: example.test\r\nContent-Type: " + type
        + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

inline std::string response(const std::string &body)
{
    return "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

inline void rejected(Client &client, const FeedResult &result)
{
    require(result.rejected && client.getParseResult() == Parser::ParseResult::ERR_PARSING,
            "Malformed response was accepted or left pending at EOF");
}

inline void rejected(Server &server, const FeedResult &result, unsigned expectedStatus = 0)
{
    require(server.contentCalls == 0, "Malformed request reached the application content callback");
    const auto code = static_cast<unsigned>(server.serverResponse.status.getCode());
    if (expectedStatus)
    {
        require(!result.rejected && server.writeStatus.finished && code == expectedStatus,
                "Expected HTTP rejection status " + std::to_string(expectedStatus) + ", got " + std::to_string(code));
        require(server.response().find(" " + std::to_string(expectedStatus) + " ") != std::string::npos,
                "HTTP rejection was not written to the response stream");
    }
    else
    {
        require((result.rejected && server.getParseResult() == Parser::ParseResult::ERR_PARSING)
                    || (server.writeStatus.finished && code >= 400 && code <= 599),
                "Malformed request was accepted or left pending at EOF");
    }
}

std::vector<RawCase> clientCases(const std::string &suite);
std::vector<RawCase> serverCases(const std::string &suite);
std::vector<RawCase> streamingCases();

} // namespace HTTPParserTests
