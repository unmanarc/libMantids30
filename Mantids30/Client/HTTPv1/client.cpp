#include "client.h"
#include "responsebody.h"

#include <Mantids30/Core/Memory/b_chunks.h>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <charconv>
#include <climits>

namespace Mantids30::Client {
using namespace Mantids30::Core::Memory::Streams;
using namespace Mantids30::Network::Sockets;
using Mantids30::Protocol::HTTP::HTTPv1_Client;
using Mantids30::Protocol::HTTP::HTTPv1_Base;
using Mantids30::Protocol::HTTP::Content;
using Mantids30::Protocol::HTTP::Status;

namespace Detail {

struct Endpoint
{
    bool secure = false;
    std::string host;
    std::string authority;
    std::string target;
    uint16_t port = 80;
};

bool isToken(const std::string &value)
{
    return !value.empty() && value.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~") == std::string::npos;
}

bool validFieldValue(const std::string &value)
{
    return std::none_of(value.begin(), value.end(), [](unsigned char c)
    {
        return (c < 32 && c != '\t') || c == 127;
    });
}

bool parseURL(const std::string &url, Endpoint *endpoint)
{
    if (std::any_of(url.begin(), url.end(), [](unsigned char c) { return c <= 32 || c == 127 || c == '\\'; }))
    {
        return false;
    }
    const size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos)
    {
        return false;
    }
    const std::string scheme = boost::to_lower_copy(url.substr(0, schemeEnd));
    if (scheme != "http" && scheme != "https")
    {
        return false;
    }
    endpoint->secure = scheme == "https";
    endpoint->port = endpoint->secure ? 443 : 80;
    const size_t authorityEnd = url.find_first_of("/?#", schemeEnd + 3);
    endpoint->authority = url.substr(schemeEnd + 3, authorityEnd == std::string::npos ? std::string::npos : authorityEnd - schemeEnd - 3);
    const std::string &authority = endpoint->authority;
    if (authority.empty() || authority.find('@') != std::string::npos)
    {
        return false;
    }
    size_t portOffset = std::string::npos;
    if (authority.front() == '[')
    {
        const size_t close = authority.find(']');
        if (close == std::string::npos || close == 1)
        {
            return false;
        }
        endpoint->host = authority.substr(1, close - 1);
        if (close + 1 < authority.size())
        {
            if (authority[close + 1] != ':')
            {
                return false;
            }
            portOffset = close + 2;
        }
    }
    else
    {
        const size_t colon = authority.find(':');
        endpoint->host = authority.substr(0, colon);
        if (colon != std::string::npos)
        {
            portOffset = colon + 1;
        }
    }
    if (endpoint->host.empty())
    {
        return false;
    }
    if (portOffset != std::string::npos)
    {
        unsigned int port = 0;
        const auto parsed = std::from_chars(authority.data() + portOffset, authority.data() + authority.size(), port);
        if (parsed.ec != std::errc() || parsed.ptr != authority.data() + authority.size() || !port || port > 65535)
        {
            return false;
        }
        endpoint->port = static_cast<uint16_t>(port);
    }
    endpoint->target = authorityEnd == std::string::npos ? "/" : url.substr(authorityEnd);
    endpoint->target = endpoint->target.substr(0, endpoint->target.find('#'));
    if (endpoint->target.empty() || endpoint->target.front() == '?')
    {
        endpoint->target.insert(0, "/");
    }
    return true;
}

class ClientProtocol : public HTTPv1_Client
{
public:
    explicit ClientProtocol(const std::shared_ptr<StreamableObject> &socket) : HTTPv1_Client(socket) {}
    std::function<bool()> headersCallback;

protected:
    bool onResponseHeaders() override { return !headersCallback || headersCallback(); }
};

// Close even when a user callback throws while the response still owns the parser.
struct ConnectionGuard
{
    std::shared_ptr<Socket_Stream> socket;
    void close()
    {
        if (socket)
        {
            socket->closeSocket();
            socket.reset();
        }
    }
    ~ConnectionGuard() { close(); }
};

} // namespace Detail

HTTPv1::HTTPv1(const Config &config) : m_config(config)
{
}

uint16_t HTTPv1::Response::getStatusCode() const
{
    return protocol ? static_cast<uint16_t>(protocol->serverResponse.status.getCode()) : 0;
}

bool HTTPv1::Response::isHTTPSuccess() const
{
    return succeed() && getStatusCode() >= 200 && getStatusCode() < 300;
}

std::string HTTPv1::Response::getHeader(const std::string &name) const
{
    return protocol ? protocol->serverResponse.headers.getOptionRawStringByName(name) : "";
}

HTTPv1::Response HTTPv1::perform(const Request &request, const Callbacks &callbacks) const
{
    Response response;
    auto finish = [&](Error error, const std::string &message)
    {
        response.error = error;
        response.errorMessage = message;
        if (callbacks.onFinished)
        {
            callbacks.onFinished(response);
        }
        return response;
    };

    Detail::Endpoint endpoint;
    if (!Detail::parseURL(request.url, &endpoint) || !Detail::isToken(request.method) || !Detail::validFieldValue(request.contentType) ||
        (!request.connection && (m_config.readTimeout > INT_MAX || m_config.writeTimeout > INT_MAX)))
    {
        return finish(Error::INVALID_REQUEST, "Invalid URL, method, content type or timeout.");
    }
    for (const auto &header : request.headers)
    {
        if (!Detail::isToken(header.first) || !Detail::validFieldValue(header.second))
        {
            return finish(Error::INVALID_REQUEST, "Invalid request header.");
        }
    }

    std::shared_ptr<StreamableObject> storage = request.responseContent;
    if (!storage)
    {
        auto chunks = std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>();
        chunks->setMaxSize(m_config.maxResponseSize);
        chunks->setMaxSizeInMemoryBeforeMovingToDisk(m_config.maxResponseMemory);
        storage = chunks;
    }
    response.content = storage;
    auto receiver = std::make_shared<Detail::HTTPv1ResponseBody>(storage, m_config.maxResponseSize, callbacks.onData, callbacks.onProgress);

    std::shared_ptr<Socket_Stream> socket = request.connection;
    if (socket)
    {
        if (!socket->isActive())
        {
            return finish(Error::CONNECTION, "Injected connection is not active.");
        }
        if (socket->isSecure() != endpoint.secure)
        {
            return finish(Error::INVALID_REQUEST, "URL scheme does not match the injected connection transport.");
        }
    }
    else
    {
        std::shared_ptr<Socket_TCP> tcp;
        if (endpoint.secure)
        {
            auto tls = std::make_shared<Socket_TLS>();
            tls->tlsKeys.setUseSystemCertificates(true);
            tls->tlsKeys.setValidateServerHostname(true);
            if (m_config.configureTLS)
            {
                m_config.configureTLS(*tls);
            }
            tcp = tls;
        }
        else
        {
            tcp = std::make_shared<Socket_TCP>();
        }
        tcp->setUseIPv6(true);
        tcp->overrideReadTimeout(static_cast<int32_t>(m_config.readTimeout));
        tcp->overrideWriteTimeout(static_cast<int32_t>(m_config.writeTimeout));
        socket = tcp;
    }
    Detail::ConnectionGuard guard{request.connection ? nullptr : socket};

    auto protocol = std::make_shared<Detail::ClientProtocol>(socket);
    response.protocol = protocol;
    protocol->setClientRequest(endpoint.host, endpoint.target);
    protocol->clientRequest.virtualPort = endpoint.port;
    protocol->clientRequest.requestLine.setHTTPMethod(boost::to_upper_copy(request.method));
    protocol->clientRequest.headers.replace("Host", endpoint.authority);
    for (const auto &header : request.headers)
    {
        protocol->clientRequest.headers.replace(header.first, header.second);
    }
    if (request.content)
    {
        protocol->clientRequest.content.setStreamableObject(request.content);
    }
    if (!request.contentType.empty())
    {
        protocol->clientRequest.headers.replace("Content-Type", request.contentType);
    }
    if (request.configure)
    {
        request.configure(*protocol);
    }
    if (!request.connection)
    {
        protocol->clientRequest.headers.replace("Connection", "close");
    }
    else if (!protocol->clientRequest.headers.exist("Connection"))
    {
        protocol->clientRequest.headers.replace("Connection", "keep-alive");
    }
    // Reception is bounded by the internal receiver, including responses without Content-Length.
    protocol->serverResponse.content.setSecurityMaxPostDataSize(std::numeric_limits<size_t>::max());
    protocol->serverResponse.content.setSecurityMaxHttpChunkSize(m_config.maxChunkSize);
    protocol->serverResponse.content.setStreamableObject(receiver);

    bool cancelled = false;
    bool contentLimit = false;
    protocol->headersCallback = [&]()
    {
        const auto &metadata = protocol->serverResponse;
        if (metadata.content.getTransmissionMode() == Content::TransmissionMode::CONTENT_LENGTH &&
            metadata.headers.exist("Content-Length"))
        {
            const size_t length = static_cast<size_t>(metadata.headers.getOptionAsUINT64("Content-Length"));
            receiver->setExpectedSize(length);
            if (length > m_config.maxResponseSize)
            {
                contentLimit = true;
                return false;
            }
        }
        cancelled = callbacks.onHeaders && !callbacks.onHeaders(metadata);
        return !cancelled;
    };

    if (!request.connection && !socket->connectTo(endpoint.host.c_str(), endpoint.port, m_config.connectTimeout))
    {
        protocol->headersCallback = {};
        std::string message = socket->getLastError();
        if (auto tls = std::dynamic_pointer_cast<Socket_TLS>(socket))
        {
            for (const auto &error : tls->getTLSErrorsAndClear())
            {
                message += " " + error;
            }
        }
        guard.close();
        return finish(Error::CONNECTION, message.empty() ? "Connection failed." : message);
    }

    Parser::ParseResult result;
    protocol->parseObject(&result);
    protocol->headersCallback = {};
    guard.close();
    if (cancelled || receiver->isCancelled())
    {
        return finish(Error::CANCELLED, "Reception cancelled by callback.");
    }
    if (contentLimit || receiver->isLimitExceeded())
    {
        return finish(Error::CONTENT_LIMIT, "Response exceeds the content size limit.");
    }
    if (result == Parser::ParseResult::ERR_INIT)
    {
        return finish(Error::SEND, "Failed to send the HTTP request.");
    }
    if (result != Parser::ParseResult::SUCCEED || !protocol->writeStatus.finished ||
        !protocol->writeStatus.succeed || !receiver->writeStatus.finished || !receiver->writeStatus.succeed)
    {
        return finish(Error::RECEIVE, "Incomplete or invalid HTTP response, or destination write failed.");
    }
    return finish(Error::NONE, "");
}

} // namespace Mantids30::Client
