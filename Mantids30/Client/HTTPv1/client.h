#pragma once

#include <Mantids30/Network/Sockets/socket_tls.h>
#include <Mantids30/Protocol/HTTP/httpv1_client.h>
#include <map>
#include <functional>

namespace Mantids30::Client {

/**
 * @brief Blocking HTTP/1.1 client. Locally created connections are owned and closed by the client.
 * Redirects and cookies are returned for explicit handling by the caller.
 * No shared connection or cookie state is retained between calls.
 */
class HTTPv1
{
public:
    struct Request
    {
        std::string url;
        std::string method = "GET";
        std::map<std::string, std::string> headers;
        std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> content;
        std::string contentType;
        /** @brief Destination written incrementally during reception. If unset, a memory B_Chunks is created. */
        std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> responseContent;
        /**
         * @brief Optional already connected socket, including a completed TLS handshake for HTTPS.
         * Borrowed without reconnecting, reconfiguring or closing it. The caller controls its lifetime.
         * The URL still supplies Host and the request target; its scheme must match the transport.
         */
        std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> connection;

        /** @brief Configure authentication, cookies, query variables, JSON, URL or MIME bodies before sending. */
        std::function<void(Mantids30::Protocol::HTTP::HTTPv1_Client &)> configure;
    };

    enum class Error : uint8_t
    {
        NONE,
        INVALID_REQUEST,
        CONNECTION,
        SEND,
        RECEIVE,
        CONTENT_LIMIT,
        CANCELLED
    };

    struct Response
    {
        Error error = Error::NONE;
        std::string errorMessage;
        /** @brief The actual reception destination: request.responseContent or the default B_Chunks. */
        std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> content;
        /** @brief Owns parsed status, repeated headers, cookies, cache, ETag and security metadata. */
        std::shared_ptr<Mantids30::Protocol::HTTP::HTTPv1_Client> protocol;

        /** @brief Transport success; HTTP errors such as 404 still count as received responses. */
        bool succeed() const { return error == Error::NONE; }
        uint16_t getStatusCode() const;
        bool isHTTPSuccess() const;
        std::string getHeader(const std::string &name) const;
    };

    struct Callbacks
    {
        /** @brief Return false from a reception callback to cancel. Data is valid only during its callback. */
        std::function<bool(const Mantids30::Protocol::HTTP::HTTPv1_Base::Response &)> onHeaders;
        std::function<bool(const void *, size_t)> onData;
        /** @brief Expected size is absent for chunked and connection-close responses. */
        std::function<bool(size_t, std::optional<size_t>)> onProgress;
        std::function<void(const Response &)> onFinished;
    };

    struct Config
    {
        uint32_t connectTimeout = 30;
        uint32_t readTimeout = 30;
        uint32_t writeTimeout = 30;
        size_t maxResponseSize = 64 * 1024 * 1024;
        /** @brief Default B_Chunks spill threshold. Zero keeps the response in memory. */
        size_t maxResponseMemory = 0;
        uint32_t maxChunkSize = 16 * 1024 * 1024;

        /** @brief Customize TLS keys, CA, mutual TLS, PSK or ciphers before connecting. Defaults verify system CA and hostname. */
        std::function<void(Mantids30::Network::Sockets::Socket_TLS &)> configureTLS;
    };

    HTTPv1() = default;
    explicit HTTPv1(const Config &config);

    /**
     * @brief Receive a complete response before returning. Callbacks run on the calling thread.
     * Bytes are written directly to request.responseContent as they arrive, or to a default memory B_Chunks.
     * Callback exceptions propagate; locally created connections are closed during unwinding.
     * Injected connections remain caller-owned, including on cancellation or failure.
     * Use fresh writable streams for each call. Custom sinks determine whether content can be replayed.
     */
    Response perform(const Request &request, const Callbacks &callbacks = {}) const;

private:
    Config m_config;
};

} // namespace Mantids30::Client
