#include <Mantids30/Client/HTTPv1/client.h>
#include <Mantids30/Core/Memory/b_chunks.h>
#include <Mantids30/Core/Memory/streamable_null.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace Mantids30;
using Request = Client::HTTPv1::Request;
using Response = Client::HTTPv1::Response;
using Config = Client::HTTPv1::Config;
using Callbacks = Client::HTTPv1::Callbacks;
using Error = Client::HTTPv1::Error;
using namespace Mantids30::Core::Memory::Streams;
using namespace Mantids30::Network::Sockets;
using Mantids30::Protocol::HTTP::HTTPv1_Client;
using Mantids30::Protocol::HTTP::HTTPv1_Base;
using Mantids30::Protocol::HTTP::Content;
using Mantids30::Protocol::HTTP::Status;

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class Server
{
public:
    Server(const std::string &response, bool fragmented = false)
    {
        require(m_listener.listenOn(0, "127.0.0.1"), "Cannot listen on loopback");
        port = m_listener.getLocalPort();
        m_thread = std::thread([this, response, fragmented]()
        {
            auto socket = m_listener.acceptConnection();
            if (!socket)
            {
                return;
            }
            socket->setReadTimeout(2);
            socket->setWriteTimeout(2);
            char buffer[4096];
            for (;;)
            {
                const auto count = socket->partialRead(buffer, sizeof(buffer));
                if (count <= 0)
                {
                    break;
                }
                received.append(buffer, static_cast<size_t>(count));
                const size_t end = received.find("\r\n\r\n");
                if (end == std::string::npos)
                {
                    continue;
                }
                const std::string headers = boost::to_lower_copy(received.substr(0, end));
                if (headers.find("transfer-encoding: chunked") != std::string::npos)
                {
                    if (received.find("\r\n0\r\n\r\n", end) != std::string::npos)
                    {
                        break;
                    }
                    continue;
                }
                const size_t lengthHeader = headers.find("content-length:");
                const size_t length = lengthHeader == std::string::npos ? 0 : std::stoull(headers.substr(lengthHeader + 15));
                if (received.size() >= end + 4 + length)
                {
                    break;
                }
            }
            if (fragmented)
            {
                for (char c : response)
                {
                    if (!socket->writeFull(&c, 1))
                    {
                        break;
                    }
                }
            }
            else
            {
                socket->writeFull(response.data(), response.size());
            }
            socket->closeSocket();
        });
    }

    ~Server() { join(); }
    void join() { if (m_thread.joinable()) m_thread.join(); }
    std::string url() const { return "http://127.0.0.1:" + std::to_string(port) + "/path?original=1#fragment"; }
    uint16_t port;
    std::string received;

private:
    Socket_TCP m_listener;
    std::thread m_thread;
};

class FailingSink : public StreamableObject
{
public:
    std::optional<size_t> write(const void *, const size_t &) override { return std::nullopt; }
};

class ZeroSink : public StreamableObject
{
public:
    std::optional<size_t> write(const void *, const size_t &) override { return 0; }
};

class FailingEOFSink : public StreamableString
{
public:
    std::optional<size_t> write(const void *data, const size_t &count) override
    {
        return count ? StreamableString::write(data, count) : std::nullopt;
    }
};

std::string decodeChunked(const std::string &wire)
{
    size_t offset = wire.find("\r\n\r\n") + 4;
    std::string body;
    for (;;)
    {
        const size_t end = wire.find("\r\n", offset);
        require(end != std::string::npos, "Chunk header delimiter");
        const size_t length = std::stoull(wire.substr(offset, end - offset), nullptr, 16);
        offset = end + 2;
        if (!length)
        {
            return body;
        }
        require(length <= wire.size() - offset && wire.substr(offset + length, 2) == "\r\n", "Request chunk length");
        body.append(wire, offset, length);
        offset += length + 2;
    }
}

Response receiveResponse(const std::string &wire, const Config &config = {}, const Callbacks &callbacks = {},
                  const std::string &method = "GET", const std::shared_ptr<StreamableObject> &sink = nullptr)
{
    Server server(wire, true);
    Request request;
    request.url = server.url();
    request.method = method;
    request.responseContent = sink;
    return Client::HTTPv1(config).perform(request, callbacks);
}

void runTrailerTests()
{
    using ParseResult = SubParser::ParseResult;
    const std::string body = "2\r\nhi\r\n0\r\n";
    const std::string trailers = "X-Trailer: value\r\nraw bytes\r\n\r\n";
    const std::string message = body + trailers;
    for (const bool fragmented : {false, true})
    {
        Content content;
        auto destination = std::make_shared<StreamableString>();
        content.setTrailerBytesContent(destination);
        require(content.getTrailerBytesContent() == destination, "Configured trailer destination");
        content.setTransmissionMode(Content::TransmissionMode::CHUNKS);
        const std::string wire = message + "NEXT MESSAGE\r\n";
        size_t offset = 0;
        while (content.getParseStatus() == ParseResult::GET_MORE_DATA && offset < wire.size())
        {
            const auto consumed = content.writeIntoSubParser(wire.data() + offset, fragmented ? 1 : wire.size() - offset);
            require(consumed && *consumed > 0, "Trailer parser makes progress");
            offset += *consumed;
        }
        require(content.getParseStatus() == ParseResult::GOTO_NEXT_SUBPARSER && offset == message.size(), "Leave next message untouched");
        require(content.getStreamableObject()->toString() == "hi", "Trailers stay separate from body");
        require(destination->toString() == trailers && destination->writeStatus.finished, "Preserve raw trailers, CRLF and EOF");
    }

    auto parse = [&](Content &content, const std::string &wire)
    {
        content.setTransmissionMode(Content::TransmissionMode::CHUNKS);
        for (const char &byte : wire)
        {
            if (content.getParseStatus() != ParseResult::GET_MORE_DATA)
            {
                break;
            }
            require(content.writeIntoSubParser(&byte, 1).has_value(), "Feed chunked bytes");
        }
    };

    Content discarded;
    discarded.setTrailerBytesContent(std::make_shared<StreamableString>());
    discarded.setTrailerBytesContent(nullptr);
    require(std::dynamic_pointer_cast<StreamableNull>(discarded.getTrailerBytesContent()) != nullptr, "Reset trailer destination to null sink");
    parse(discarded, message);
    require(discarded.getParseStatus() == ParseResult::GOTO_NEXT_SUBPARSER, "Discard trailers by default");

    Content empty;
    auto destination = std::make_shared<StreamableString>();
    empty.setTrailerBytesContent(destination);
    parse(empty, body + "\r\n");
    require(empty.getParseStatus() == ParseResult::GOTO_NEXT_SUBPARSER && destination->toString() == "\r\n" && destination->writeStatus.finished, "Empty trailer section");

    for (const auto &sink : {std::shared_ptr<StreamableObject>(std::make_shared<FailingSink>()), std::shared_ptr<StreamableObject>(std::make_shared<FailingEOFSink>())})
    {
        Content failed;
        failed.setTrailerBytesContent(sink);
        parse(failed, message);
        require(failed.getParseStatus() == ParseResult::ERROR_STREAM_WRITE, "Propagate trailer write and EOF failures");
    }

    for (const auto &wire : {body, body + "X-Trailer: incomplete", body + "X-Trailer: value\r\n"})
    {
        Content truncated;
        parse(truncated, wire);
        truncated.writeIntoSubParser(nullptr, 0);
        require(truncated.getParseStatus() == ParseResult::ERROR, "Reject missing trailer terminator");
    }
}

void runDestinationTests()
{
    const std::string ok = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    require(receiveResponse(ok, {}, {}, "GET", std::make_shared<FailingSink>()).error == Error::STREAM_WRITE, "Destination write failure");
    require(receiveResponse(ok, {}, {}, "GET", std::make_shared<ZeroSink>()).error == Error::STREAM_WRITE, "Zero-progress destination");
    require(receiveResponse(ok, {}, {}, "GET", std::make_shared<FailingEOFSink>()).error == Error::STREAM_WRITE, "Destination EOF failure");
    for (const auto &wire : {std::string("HTTP/1.0 200 OK\r\n\r\nhello"),
                             std::string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n")})
    {
        for (const auto &sink : {std::shared_ptr<StreamableObject>(std::make_shared<FailingSink>()),
                                 std::shared_ptr<StreamableObject>(std::make_shared<ZeroSink>()),
                                 std::shared_ptr<StreamableObject>(std::make_shared<FailingEOFSink>())})
        {
            const auto response = receiveResponse(wire, {}, {}, "GET", sink);
            require(response.error == Error::STREAM_WRITE, "Destination failure for " + wire.substr(0, wire.find("\r\n")) + ": " + response.errorMessage + " (" + typeid(*sink).name() + ")");
            require(response.protocol->getParseResult() == Parser::ParseResult::ERR_STREAM_WRITE, "Preserve parser write error");
        }
    }
    require(receiveResponse("HTTP/1.1 204 No Content\r\n\r\n", {}, {}, "GET", std::make_shared<FailingEOFSink>()).error == Error::STREAM_WRITE,
            "Empty response destination EOF failure");
}

void runTests()
{
    const std::string ok = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nSet-Cookie: session=value; Path=/\r\nETag: \"abc\"\r\n\r\nhello";
    std::string events;
    size_t progress = 0;
    Callbacks callbacks;
    callbacks.onHeaders = [&](const HTTPv1_Base::Response &metadata)
    {
        events += 'H';
        return metadata.status.getCode() == Status::Code::S_200_OK;
    };
    callbacks.onData = [&](const void *, size_t) { events += 'D'; return true; };
    callbacks.onProgress = [&](size_t received, std::optional<size_t> expected)
    {
        progress = received;
        require(expected == 5, "Expected content length in progress callback");
        return true;
    };
    callbacks.onFinished = [&](const Response &response) { events += 'F'; require(response.succeed(), response.errorMessage + " body=" + std::to_string(response.content->size()) + " bodyOK=" + std::to_string(response.content->writeStatus.succeed) + " bodyEOF=" + std::to_string(response.content->writeStatus.finished)); };
    auto response = receiveResponse(ok, {}, callbacks);
    require(response.isHTTPSuccess() && response.content->toString() == "hello", "Fixed-length response");
    auto defaultContent = std::dynamic_pointer_cast<Mantids30::Core::Memory::Containers::B_Chunks>(response.content);
    require(defaultContent && !defaultContent->isUsingFiles(), "Default destination is memory B_Chunks");
    require(progress == 5 && events.front() == 'H' && events.back() == 'F', "Callback order and progress");
    require(response.protocol->serverResponse.cookies.getCookieValueByName("session") == "value", "Set-Cookie parsing");
    require(response.protocol->getResponseETag().toString() == "\"abc\"", "ETag parsing");
    StreamableString replay;
    require(response.content->streamTo(&replay) && replay.getValue() == "hello", "Replay after connection closes");

    for (int mode = 0; mode < 3; ++mode)
    {
        Server server(ok);
        auto connection = std::make_shared<Socket_TCP>();
        require(connection->connectTo("127.0.0.1", server.port), "Connect injected TCP socket");
        Request request;
        request.url = "http://virtual.invalid/data";
        request.connection = connection;
        Callbacks injectedCallbacks;
        if (mode == 1)
        {
            injectedCallbacks.onData = [](const void *, size_t) { return false; };
        }
        else if (mode == 2)
        {
            injectedCallbacks.onHeaders = [](const HTTPv1_Base::Response &) -> bool { throw std::runtime_error("Injected callback exception"); };
        }
        bool threw = false;
        try
        {
            response = Client::HTTPv1().perform(request, injectedCallbacks);
        }
        catch (const std::runtime_error &error)
        {
            threw = std::string(error.what()) == "Injected callback exception";
        }
        require(connection->isActive(), "Injected connection remains caller-owned");
        connection->closeSocket();
        server.join();
        require(server.received.find("Host: virtual.invalid\r\n") != std::string::npos, "Injected socket uses URL authority without reconnecting or DNS");
        require(server.received.find("Connection: keep-alive\r\n") != std::string::npos, "Injected socket defaults to keep-alive");
        if (mode == 0)
        {
            require(response.succeed() && response.content->toString() == "hello", "Injected TCP response");
        }
        else if (mode == 1)
        {
            require(response.error == Error::CANCELLED, "Injected TCP cancellation");
        }
        else
        {
            require(threw, "Injected callback exception propagates");
        }
    }
    {
        Request request;
        request.url = "http://virtual.invalid/";
        request.connection = std::make_shared<Socket_TCP>();
        require(Client::HTTPv1().perform(request).error == Error::CONNECTION, "Inactive injected socket rejected without connecting");
    }

    response = receiveResponse("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Length: 99\r\n\r\n2\r\nhe\r\n3\r\nllo\r\n0\r\nX-Trailer: value\r\n\r\n");
    require(response.succeed() && response.content->toString() == "hello", "Chunked response with trailers: " + response.errorMessage);
    response = receiveResponse("HTTP/1.0 200 OK\r\n\r\nhello");
    require(response.succeed() && response.content->toString() == "hello", "Connection-close response");
    response = receiveResponse("HTTP/1.1 103 Early Hints\r\nLink: </style>\r\n\r\nHTTP/1.1 100 Continue\r\n\r\n" + ok);
    require(response.succeed() && response.getStatusCode() == 200 && response.getHeader("Link").empty(), "Informational responses");

    for (const auto &wire : {"HTTP/1.1 204 No Content\r\n\r\n", "HTTP/1.1 304 Not Modified\r\nContent-Length: 42\r\n\r\n", "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"})
    {
        response = receiveResponse(wire);
        require(response.succeed() && response.content->size() == 0, "Response without body");
    }
    response = receiveResponse("HTTP/1.1 200 OK\r\nContent-Length: 99999\r\n\r\n", {}, {}, "HEAD");
    require(response.succeed() && response.content->size() == 0, "HEAD ignores representation length");
    response = receiveResponse("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
    require(response.succeed() && !response.isHTTPSuccess() && response.getStatusCode() == 404, "HTTP versus transport errors");

    for (const auto &wire : {"HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nshort", "HTTP/1.1 200 OK\r\nContent-Length: bad\r\n\r\n", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n8\r\nshort", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhiXX0\r\n\r\n", "HTTP/1.1 200 OK\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nx", "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n"})
    {
        response = receiveResponse(wire);
        require(response.error == Error::RECEIVE, "Reject malformed or truncated response");
    }
    Config config;
    config.maxResponseSize = 4;
    require(receiveResponse(ok, config).error == Error::CONTENT_LIMIT, "Declared content limit");
    require(receiveResponse("HTTP/1.0 200 OK\r\n\r\nhello", config).error == Error::CONTENT_LIMIT, "Connection-close content limit");
    require(receiveResponse("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n", config).error == Error::CONTENT_LIMIT, "Chunked content limit");
    config = Config();
    config.maxResponseMemory = 1;
    response = receiveResponse(ok, config);
    auto chunks = std::dynamic_pointer_cast<Mantids30::Core::Memory::Containers::B_Chunks>(response.content);
    require(response.succeed() && chunks->isUsingFiles() && response.content->toString() == "hello", "Disk spillover: " + response.errorMessage + " stored=" + std::to_string(chunks->size()) + " body=" + response.content->toString());
    callbacks = Callbacks();
    callbacks.onData = [](const void *, size_t) { return false; };
    require(receiveResponse(ok, {}, callbacks).error == Error::CANCELLED, "Data callback cancellation");
    callbacks = Callbacks();
    callbacks.onHeaders = [](const HTTPv1_Base::Response &) { return false; };
    require(receiveResponse(ok, {}, callbacks).error == Error::CANCELLED, "Header callback cancellation");
    callbacks = Callbacks();
    callbacks.onProgress = [](size_t, std::optional<size_t>) { return false; };
    require(receiveResponse(ok, {}, callbacks).error == Error::CANCELLED, "Progress callback cancellation");
    runDestinationTests();
    auto sink = std::make_shared<StreamableString>();
    response = receiveResponse(ok, {}, {}, "GET", sink);
    require(response.succeed() && response.content == sink && sink->getValue() == "hello", "Custom destination identity");

    auto liveContent = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
    callbacks = Callbacks();
    size_t liveBytes = 0;
    callbacks.onProgress = [&](size_t received, std::optional<size_t>)
    {
        require(liveContent->size() == received, "Destination is updated during reception");
        liveBytes = received;
        return true;
    };
    response = receiveResponse(ok, {}, callbacks, "GET", liveContent);
    require(response.succeed() && response.content == liveContent && liveBytes == 5 && liveContent->toString() == "hello", "Direct reception into caller B_Chunks");

    for (int kind = 0; kind < 4; ++kind)
    {
        Server server(ok);
        Request request;
        request.url = server.url();
        request.method = "POST";
        request.configure = [kind](HTTPv1_Client &protocol)
        {
            protocol.setBasicAuthentication("user", "pass");
            protocol.addCookie("session", "abc");
            protocol.addURLVar("added", "two words");
            if (kind == 0)
            {
                protocol.clientRequest.content.setStreamableObject(std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>(std::string("a\0b", 3)));
            }
            else if (kind == 1)
            {
                protocol.clientRequest.content.setContainerType(Content::ContainerType::JSON);
                protocol.clientRequest.content.getJSONVars()->setValue(boost::json::value(boost::json::object{{"test", true}}));
            }
            else if (kind == 2)
            {
                protocol.clientRequest.content.setContainerType(Content::ContainerType::URL);
                protocol.clientRequest.content.getUrlPostVars()->addVar("name", std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>("hello"));
            }
            else
            {
                protocol.clientRequest.content.setContainerType(Content::ContainerType::MIME);
                auto mime = protocol.clientRequest.content.getMultiPartVars();
                require(mime->addStringVar("name", "hello"), "Add multipart field");
                require(mime->addStringVar("second", "world"), "Add second multipart field");
                require(mime->addReferecedFileVar("file", __FILE__), "Add multipart file");
            }
        };
        response = Client::HTTPv1().perform(request);
        server.join();
        require(response.succeed(), "Upload: " + response.errorMessage);
        const auto sent = boost::to_lower_copy(server.received);
        require(sent.find("authorization: basic dxnlcjpwyxnz") != std::string::npos, "Basic authorization header");
        require(sent.find("session=abc") != std::string::npos, "Request cookies");
        const std::string requestLine = sent.substr(0, sent.find("\r\n"));
        require(requestLine.find("/path?original=1&added=two") != std::string::npos && requestLine.find("fragment") == std::string::npos, "Query merge and fragment removal");
        const char *types[] = {"content-length: 3", "application/json", "application/x-www-form-urlencoded", "multipart/form-data; boundary="};
        require(sent.find(types[kind]) != std::string::npos, "Upload content type and framing " + std::to_string(kind) + ": " + sent);
        if (kind == 3)
        {
            require(sent.find("transfer-encoding: chunked") != std::string::npos && sent.find("hello") != std::string::npos, "Chunked multipart upload");
            const std::string body = decodeChunked(server.received);
            const auto outgoing = response.protocol->clientRequest.content.getMultiPartVars();
            auto incoming = Mantids30::Protocol::MIME::MIME_Message::create();
            incoming->setMultiPartBoundary(outgoing->getMultiPartBoundary());
            require(incoming->writeFullStream(body.data(), body.size()) && incoming->writeStatus.finished, "Parse multipart upload");
            require(incoming->getValue("name")->toString() == "hello" && incoming->getValue("second")->toString() == "world", "Multipart values");
            require(incoming->getFirstMessageByName("file")->getHeader()->getOptionByName("Content-Disposition")->getSubVar("filename") == "client_test.cpp", "Multipart filename");
        }
    }
    for (const auto &url : {"ftp://example.com/", "http://user:pass@example.com/", "http://example.com:0/", "http://example.com:bad/", "http://example.com/\r\nx"})
    {
        Request request;
        request.url = url;
        require(Client::HTTPv1().perform(request).error == Error::INVALID_REQUEST, "Invalid URL rejection");
    }
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        if (argc == 2 && std::string(argv[1]) == "--trailers")
        {
            runTrailerTests();
            std::cout << "HTTP trailer tests passed\n";
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "--tls")
        {
            Config config;
            const std::string caPath = argv[3];
            if (caPath != "-")
            {
                config.configureTLS = [&](Socket_TLS &socket)
                {
                    require(socket.tlsKeys.loadCAFromPEMFile(caPath), "Load test CA");
                };
            }
            Request request;
            request.url = argv[2];
            const bool injected = std::string(argv[4]) == "injected";
            if (injected)
            {
                const auto colon = request.url.find_last_of(':');
                const auto port = static_cast<uint16_t>(std::stoul(request.url.substr(colon + 1)));
                auto connection = std::make_shared<Socket_TLS>();
                connection->tlsKeys.setUseSystemCertificates(true);
                connection->tlsKeys.setValidateServerHostname(true);
                require(connection->tlsKeys.loadCAFromPEMFile(caPath), "Load CA for injected TLS connection");
                require(connection->connectTo("localhost", port), "Connect injected TLS socket");
                request.connection = connection;
                request.url = "https://virtual.invalid/data";
                config.configureTLS = [](Socket_TLS &) { throw std::runtime_error("Injected TLS must not be configured again"); };
            }
            auto response = Client::HTTPv1(config).perform(request);
            if (std::string(argv[4]) == "success" || injected)
            {
                require(response.succeed() && response.content->toString() == "hello", "TLS request: " + response.errorMessage);
                if (injected)
                {
                    require(request.connection->isActive(), "Injected TLS socket remains caller-owned");
                    request.connection->closeSocket();
                }
            }
            else
            {
                require(response.error == Error::CONNECTION, "Reject untrusted certificate or wrong hostname");
            }
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--write-errors")
        {
            runDestinationTests();
            return 0;
        }
        runTests();
        std::cout << "HTTPv1 integration tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
