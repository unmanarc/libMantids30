#pragma once

#include <Mantids30/Server/WebRESTFul/engine.h>
#include <Mantids30/Protocol/HTTP/httpv1_client.h>
#include <Mantids30/Network/Sockets/socket_stream_dummy.h>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace WebRESTFulTests {

namespace Web = Mantids30::Server::WebRESTFul;
namespace HTTP = Mantids30::Protocol::HTTP;
using JWT = Mantids30::DataFormat::JWT;
using Endpoints = Web::RESTful::Endpoints;
using Result = Endpoints::HandleResult;
using RequestContext = Web::RESTful::RequestContext;
using HTTP::APIReturn;
using HTTP::Method;
using Code = HTTP::Status::Code;
using Mantids30::Core::Memory::Streams::StreamableString;

inline void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

struct Case
{
    std::string name;
    std::function<void()> run;
};

struct Probe
{
    size_t calls = 0;
    boost::json::value input;
    Web::Sessions::ClientDetails details;
    std::string subject;
    APIReturn reply{boost::json::object{{"ok", true}}};

    static APIReturn invoke(void *context, const RequestContext &request, Web::Sessions::ClientDetails &details)
    {
        auto &probe = *static_cast<Probe *>(context);
        ++probe.calls;
        probe.input = *request.inputJSON;
        probe.details = details;
        probe.subject = request.jwtToken->getSubject();
        return probe.reply;
    }
};

struct Fixture
{
    HTTP::HTTPv1_Base::Request request;
    RequestContext context;
    Endpoints endpoints;
    Probe probe;
    APIReturn response;

    Fixture()
    {
        context.clientRequest = &request;
        request.requestLine.setHTTPMethod("POST");
        request.networkClientInfo.setClientInformation("192.0.2.4", true, "test-client");
        request.networkClientInfo.tlsCommonName = "test-client";
        request.userAgent = "curl/8.0";
    }

    void add(Method method = Method::POST, Web::Security::Requirements security = Web::Security::NONE,
             const std::set<std::string> &scopes = {}, const std::string &path = "resource")
    {
        require(endpoints.addEndpoint(method, path, security, scopes, &probe, Probe::invoke), "Endpoint registration failed");
    }

    Result call(Method method = Method::POST, const std::string &path = "resource",
                const std::set<std::string> &scopes = {}, bool admin = false, Web::Security::ReceivedAuth auth = {})
    {
        return endpoints.handleEndpoint(method, path, context, scopes, admin, auth, &response);
    }

    void error(Result actual, Result expected, Code status, const std::string &message)
    {
        require(actual == expected, "Wrong dispatch result");
        require(response.getHTTPResponseCode() == status, "Wrong rejection status");
        require(response.responseJSON()->at("error") == "invalid_invokation", "Wrong rejection error");
        require(response.responseJSON()->at("message") == boost::json::value(message), "Wrong rejection message");
        require(probe.calls == 0, "Rejected request invoked callback");
    }
};

template<class Parser>
void feed(Parser &parser, const std::string &raw, size_t fragment = 0)
{
    size_t consumed = 0;
    while (consumed < raw.size() && !parser.writeStatus.finished)
    {
        const size_t count = std::min(fragment ? fragment : raw.size(), raw.size() - consumed);
        const auto written = parser.write(raw.data() + consumed, count);
        require(written && *written > 0 && *written <= count, "HTTP parser rejected input or stopped progressing");
        consumed += *written;
    }
    if (!parser.writeStatus.finished)
    {
        require(parser.writeEOF(), "HTTP parser rejected EOF");
    }
    require(consumed == raw.size() && parser.writeStatus.finished, "HTTP message did not complete at its boundary");
}

// Expose the normal factory without starting acceptor threads or opening ports.
class Engine : public Web::Engine
{
public:
    using Web::Engine::createNewAPIServer_ClientHandler;
};

inline std::string rawRequest(const std::string &method, const std::string &path, const std::string &body = "",
                              const std::string &headers = "", const std::string &type = "application/json")
{
    return method + " " + path + " HTTP/1.1\r\nHost: example.test\r\nUser-Agent: curl/8.0\r\nConnection: close\r\n"
        + headers + "Content-Type: " + type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

struct Reply
{
    unsigned status;
    std::string body;
    std::string raw;
};

inline Reply sendRequest(Engine &engine, const std::string &raw, size_t fragment = 0)
{
    auto socket = std::make_shared<Mantids30::Network::Sockets::Socket_Stream_Dummy>();
    auto handler = engine.createNewAPIServer_ClientHandler(&engine, socket);
    handler->config = &engine.config;
    handler->clientRequest.networkClientInfo.setClientInformation("192.0.2.4", true, "test-client");
    engine.config.webLog = std::make_shared<Mantids30::Runtime::Logs::WebLog>();
    engine.config.webLog->config.useThreadedQueue = false;
    feed(*handler, raw, fragment);
    const auto responseBuffer = socket->getReceiver()->toString();
    require(responseBuffer.has_value(), "Cannot read response buffer");
    const std::string &response = *responseBuffer;
    HTTP::HTTPv1_Client client(std::make_shared<StreamableString>());
    feed(client, response, fragment);
    return {static_cast<unsigned>(client.serverResponse.status.getCode()),
            client.serverResponse.content.getStreamableObject()->toString(), response};
}

std::vector<Case> endpointCases(const std::string &suite);
std::vector<Case> policyCases(const std::string &suite);
std::vector<Case> stateCases(const std::string &suite);
std::vector<Case> integrationCases();

} // namespace WebRESTFulTests
