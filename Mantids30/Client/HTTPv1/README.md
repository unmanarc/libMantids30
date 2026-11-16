# HTTPv1 synchronous client

`Mantids30::Client::HTTPv1` sends HTTP/1.1 requests synchronously using the
framework's HTTP parser, streamable objects and TCP/TLS sockets.

The client writes response bytes into a destination while receiving them. You
can supply your own memory container, file or custom `StreamableObject`. If you
do not supply one, it creates a `B_Chunks` memory container.

This guide documents the high-level class in [client.h](client.h). The lower-level
`Mantids30::Protocol::HTTP::HTTPv1_Client` remains available through the request
configuration callback and response metadata.

## Contents

- [Build and link](#build-and-link)
- [First request](#first-request)
- [Request and response reference](#request-and-response-reference)
- [Receive content in memory or a file](#receive-content-in-memory-or-a-file)
- [Send request bodies](#send-request-bodies)
- [Headers, query variables and authentication](#headers-query-variables-and-authentication)
- [Callbacks and cancellation](#callbacks-and-cancellation)
- [Configure limits and timeouts](#configure-limits-and-timeouts)
- [HTTPS and custom TLS settings](#https-and-custom-tls-settings)
- [Inject an existing connection](#inject-an-existing-connection)
- [Reuse a connection for multiple requests](#reuse-a-connection-for-multiple-requests)
- [Read response metadata](#read-response-metadata)
- [Errors and ownership](#errors-and-ownership)
- [Supported behavior and limits](#supported-behavior-and-limits)
- [Run the integration tests](#run-the-integration-tests)

## Build and link

The module requires C++17 and the existing libMantids30 dependencies. See the
[project build instructions](../../../README.md#build-from-source) and the
[platform guides](../../../docs/README.md) for dependency installation.

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Mantids30_Client_HTTPv1 -j4
```

The CMake target is `Mantids30_Client_HTTPv1`. When your application is built
inside a CMake tree that already defines this target:

```cmake
target_compile_features(my_application PRIVATE cxx_std_17)
target_include_directories(my_application PRIVATE /path/to/libMantids30)
target_link_libraries(my_application PRIVATE Mantids30_Client_HTTPv1)
```

For an installed libMantids30, use its pkg-config package:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(MANTIDS_HTTP REQUIRED IMPORTED_TARGET Mantids30_Client_HTTPv1)
target_compile_features(my_application PRIVATE cxx_std_17)
target_link_libraries(my_application PRIVATE PkgConfig::MANTIDS_HTTP)
```

Or compile a standalone example against the installed libraries:

```sh
c++ -std=c++17 example.cpp -o example \
    $(pkg-config --cflags --libs Mantids30_Client_HTTPv1)
```

Adjust `PKG_CONFIG_PATH` if the installation's `share/pkgconfig` directory is not
in your system's search path. Include the public API with:

```cpp
#include <Mantids30/Client/HTTPv1/client.h>
```

## First request

This complete example uses the default response destination:

```cpp
#include <Mantids30/Client/HTTPv1/client.h>
#include <iostream>

int main()
{
    using namespace Mantids30;

    Client::HTTPv1 client;
    Client::HTTPv1::Request request;
    request.url = "https://example.com/data";

    auto response = client.perform(request);
    if (!response.succeed())
    {
        std::cerr << response.errorMessage << '\n';
        return 1;
    }

    std::cout << "HTTP status: " << response.getStatusCode() << '\n';
    std::cout << response.content->toString() << '\n';
    return response.isHTTPSuccess() ? 0 : 1;
}
```

`perform()` blocks the calling thread until the request finishes or fails.
There is no worker thread or asynchronous result. The returned content is
already received; no second download or copy is required.

Unless stated otherwise, the following snippets assume these declarations and
execute inside an application function:

```cpp
using namespace Mantids30;

Client::HTTPv1 client;
Client::HTTPv1::Request request;
request.url = "https://example.com/data";
```

Include `<Mantids30/Core/Memory/b_chunks.h>` when using `B_Chunks` directly.
Other examples state their additional headers.

## Request and response reference

All public request, response, callback and configuration types belong to the
`HTTPv1` class.

### `Client::HTTPv1::Request`

| Member | Default | Meaning |
| --- | --- | --- |
| `url` | Empty | Absolute `http://` or `https://` URL. Required even with an injected connection. |
| `method` | `GET` | HTTP method, normalized to uppercase before sending. |
| `headers` | Empty map | Request headers. Names are handled case-insensitively by the HTTP parser. |
| `content` | Null | Streamable source for the outgoing request body. |
| `contentType` | Empty | Explicit Content-Type for the outgoing body. |
| `responseContent` | Null | Destination written incrementally during reception. Null selects the default memory `B_Chunks`. |
| `connection` | Null | Already connected `Socket_Stream`, optionally a `Socket_TLS` with its handshake completed. Null selects automatic connection creation. |
| `configure` | Empty callback | Access to the low-level HTTP client before transmission. |

`content` is the upload source; `responseContent` is the download destination.
They serve separate directions and should be separate objects.

URLs may include explicit ports, bracketed IPv6 addresses and query strings.
Fragments are removed before transmission. URL user information such as
`https://user:password@example.com/` is rejected; use an authentication header
or `setBasicAuthentication()` instead. Encode spaces and other characters
that need URL escaping before assigning the URL.

Ordinary request fields are applied first, then `configure` runs. The client
subsequently sets its response receiver, reception limits and connection policy.
The protocol generates Content-Length or Transfer-Encoding from the body source.

### `Client::HTTPv1::Response`

| Member or method | Meaning |
| --- | --- |
| `error` | Transport/request result as `Client::HTTPv1::Error`. |
| `errorMessage` | Diagnostic text; empty on success. |
| `content` | The actual receiving object, identical to `request.responseContent` when supplied. |
| `protocol` | Low-level HTTP client owning the parsed response metadata. |
| `succeed()` | True when the request and response completed without a client/transport error. |
| `getStatusCode()` | Parsed HTTP status, such as 200 or 404. Check transport success before relying on it. |
| `isHTTPSuccess()` | Transport success and an HTTP status in the range 200–299. |
| `getHeader(name)` | First response header value; empty when absent. |

A complete `404 Not Found` response has `succeed() == true` and
`isHTTPSuccess() == false`. An interrupted download has `succeed() == false`
and may leave partial data in the destination.

## Receive content in memory or a file

### Supply your own memory container

```cpp
auto content = std::make_shared<Core::Memory::Containers::B_Chunks>();
request.responseContent = content;

auto response = client.perform(request);
if (response.succeed())
{
    // This is the same object the client filled while receiving.
    auto bytes = content->size();
    auto text = content->toStringEx();
}
```

`response.content == content`. The HTTPv1 client itself is not a streamable
object; it writes directly to the destination you select. An internal adapter
forwards bytes and implements limits/callbacks without retaining another copy.
The HTTP parser may buffer protocol units, such as a chunk, before forwarding.

With no `responseContent`, `response.content` points to a newly created
`B_Chunks`. It stays in memory by default. To enable disk spillover for that
default container, set `Config::maxResponseMemory` to a nonzero byte threshold.
For a caller-created `B_Chunks`, configure its storage policy yourself:

```cpp
auto content = std::make_shared<Core::Memory::Containers::B_Chunks>();
content->setMaxSizeInMemoryBeforeMovingToDisk(2 * 1024 * 1024);
request.responseContent = content;
```

### Download directly into a file

Additional headers: `<Mantids30/Core/Memory/streamable_file.h>`, `<fcntl.h>`
and `<stdexcept>`.

```cpp


auto file = std::make_shared<Core::Memory::Streams::StreamableFile>(-1, -1);
if (file->open("download.bin", O_WRONLY | O_CREAT | O_TRUNC, 0600) < 0)
{
    throw std::runtime_error("Cannot open download destination");
}
request.responseContent = file;
auto response = client.perform(request);
```

The file receives body bytes as they arrive. Its stream owns the file descriptor
and closes it when destroyed. A file opened only for writing is a destination;
read the file separately if you later need to consume its contents.

### Consume callbacks without retaining content

Additional header: `<Mantids30/Core/Memory/streamable_null.h>`.

```cpp
request.responseContent =
    std::make_shared<Core::Memory::Streams::StreamableNull>();
```

Use this with `onData` for processing without storing the response. Response size
limits still apply. A sink determines its own ability to read or replay data;
`response.content` does not add storage to a write-only sink.

A successful response sends EOF to its destination. Use a fresh writable
container for each call. Clearing bytes alone may not reset a stream's finished
write status; replacing the destination avoids mixing separate responses.

## Send request bodies

### Raw text or binary

```cpp
request.method = "PUT";
request.contentType = "application/octet-stream";
request.content = std::make_shared<Core::Memory::Containers::B_Chunks>(std::string("a\0b", 3));
auto response = client.perform(request);
```

A source with a known `size()` uses Content-Length. A source returning an unknown
size uses HTTP/1.1 chunked encoding. The protocol selects the framing; do not
supply your own Content-Length or Transfer-Encoding values.

### JSON

```cpp
request.method = "POST";
request.configure = [](Protocol::HTTP::HTTPv1_Client &http)
{
    http.clientRequest.content.setContainerType(Protocol::HTTP::Content::ContainerType::JSON);
    http.clientRequest.content.getJSONVars()->setValue(
        boost::json::value(boost::json::object{{"enabled", true}, {"count", 3}}));
};
auto response = client.perform(request);
```

Content-Type defaults to `application/json` for this container.

### URL-encoded form

```cpp
request.method = "POST";
request.configure = [](Protocol::HTTP::HTTPv1_Client &http)
{
    http.clientRequest.content.setContainerType(Protocol::HTTP::Content::ContainerType::URL);
    auto form = http.clientRequest.content.getUrlPostVars();
    form->addVar("name", std::make_shared<Core::Memory::Containers::B_Chunks>("Alice Smith"));
    form->addVar("message", std::make_shared<Core::Memory::Containers::B_Chunks>("Hello"));
};
auto response = client.perform(request);
```

The container handles form encoding and the protocol supplies
`application/x-www-form-urlencoded`.

### Multipart form with a file

Additional header: `<stdexcept>`.

```cpp
request.method = "POST";
request.configure = [](Protocol::HTTP::HTTPv1_Client &http)
{
    http.clientRequest.content.setContainerType(Protocol::HTTP::Content::ContainerType::MIME);
    auto form = http.clientRequest.content.getMultiPartVars();
    if (!form->addStringVar("description", "Monthly report") ||
        !form->addReferecedFileVar("attachment", "report.pdf"))
    {
        throw std::runtime_error("Cannot prepare multipart request");
    }
};
auto response = client.perform(request);
```

`addReferecedFileVar` is the existing API spelling. It references the file,
sets a filename from its basename and uses `application/octet-stream` for the
file part. The protocol supplies multipart/form-data with the generated boundary.
Multipart streams without a fixed size are sent using chunked encoding.

Choose one outgoing body source. If you set `request.content`, it replaces the
internal body container; a later container-type change alone will not restore
an internal JSON/form/MIME container. To build one inside `configure`, leave
`request.content` unset or reset the low-level content stream first.

## Headers, query variables and authentication

```cpp
request.headers["Accept"] = "application/json";
request.headers["Authorization"] = "Bearer access-token";
request.headers["User-Agent"] = "MyApplication/1.0";
```

Use `configure` to reach the protocol helpers:

```cpp
request.configure = [](Protocol::HTTP::HTTPv1_Client &http)
{
    http.addURLVar("search", "two words");
    http.addURLVar("page", "1");
    http.addCookie("session", "session-value");
    http.setReferer("https://example.com/");
    http.setIfNoneMatch("\"cached-version\"");
};
```

Query variables are encoded and appended to any existing URL query string.
Cookies and conditional headers apply only to this request; the client keeps
no automatic cookie jar or cache.

For Basic authentication:

```cpp
request.configure = [](Protocol::HTTP::HTTPv1_Client &http)
{
    http.setBasicAuthentication("username", "password");
};
```

Choose the authentication scheme your server expects. Setting Basic
authentication generates Authorization and replaces an existing value there.
A new assignment to `request.configure` replaces its previous callback; combine
body, authentication and other configuration in one callback when needed.

## Callbacks and cancellation

Additional header for this example: `<iostream>`.

```cpp
Client::HTTPv1::Callbacks callbacks;
callbacks.onHeaders = [](const auto &metadata)
{
    return metadata.headers.getOptionRawStringByName("Protocol::HTTP::Content-Type") ==
           "application/octet-stream";
};
callbacks.onData = [](const void *data, size_t size)
{
    // Inspect/process these bytes here; the pointer is valid only in this call.
    return true;
};
callbacks.onProgress = [](size_t received, std::optional<size_t> expected)
{
    std::cout << "Received " << received;
    if (expected)
    {
        std::cout << " of " << *expected;
    }
    std::cout << " bytes\n";
    return true;
};
callbacks.onFinished = [](const Client::HTTPv1::Response &response)
{
    if (!response.succeed())
    {
        std::cerr << response.errorMessage << '\n';
    }
};

auto response = client.perform(request, callbacks);
```

| Callback | When it runs | Return value |
| --- | --- | --- |
| `onHeaders` | After final response headers and supported metadata are parsed, before receiving the body. Informational responses do not trigger it. | False cancels reception. |
| `onData` | Before each forwarded body block is written into the destination. | False rejects that block and cancels reception. |
| `onProgress` | After a block has been written into the destination. | False cancels subsequent reception; the current block is already written. |
| `onFinished` | Once before a normal success or failure return, including request validation errors. | No return value. |

All callbacks execute on the thread calling `perform()`. Expected size is
provided for Content-Length framing and is absent for chunked/connection-close
responses. Progress measures body bytes after removing transfer framing, without
Content-Encoding decompression. Empty bodies need not trigger data or progress
callbacks; use `onFinished` to observe completion.

Callback exceptions propagate to the caller. They do not become an `Error` value
and do not trigger an additional `onFinished` call. Automatically created sockets
are closed during stack unwinding; injected sockets remain under caller control.
Keep captured objects alive through the call and do not concurrently use the
same destination or socket from another request.

## Configure limits and timeouts

```cpp
Client::HTTPv1::Config config;
config.connectTimeout = 10;
config.readTimeout = 30;
config.writeTimeout = 30;
config.maxResponseSize = 256 * 1024 * 1024;
config.maxResponseMemory = 4 * 1024 * 1024;
config.maxChunkSize = 16 * 1024 * 1024;
Client::HTTPv1 configuredClient(config);
```

| Setting | Default | Scope |
| --- | --- | --- |
| `connectTimeout` | 30 seconds | TCP connection establishment for automatically created sockets. |
| `readTimeout` | 30 seconds | Socket read operations for automatically created sockets. |
| `writeTimeout` | 30 seconds | Socket write operations for automatically created sockets. |
| `maxResponseSize` | 64 MiB | Total forwarded body size, including custom destinations and unknown-length responses. |
| `maxResponseMemory` | 0 | Default `B_Chunks` spill threshold; zero disables spillover. Does not configure caller destinations. |
| `maxChunkSize` | 16 MiB | Maximum individual received HTTP chunk. Larger chunks fail parsing. |
| `configureTLS` | Empty | Customize a newly created TLS socket before connection establishment. |

Timeouts are per socket operation, not an overall request deadline. DNS lookup
uses the socket library's resolver. Injected sockets retain the timeouts you
configured before passing them to the client. `HTTPv1` copies its Config at
construction; changing the original config afterward does not reconfigure it.

## HTTPS and custom TLS settings

Automatically created HTTPS connections verify the system CA chain and the URL
hostname/IP. TLS settings can be extended with `Config::configureTLS`.

For a private CA, optionally with a client certificate and private key:

```cpp
Client::HTTPv1::Config config;
config.configureTLS = [](Network::Sockets::Socket_TLS &socket)
{
    if (!socket.tlsKeys.loadCAFromPEMFile("private-ca.pem") ||
        !socket.tlsKeys.loadPublicKeyFromPEMFile("client-certificate.pem") ||
        !socket.tlsKeys.loadPrivateKeyFromPEMFile("client-key.pem"))
    {
        throw std::runtime_error("Cannot load TLS credentials");
    }
};
Client::HTTPv1 privateClient(config);
```

Include `<stdexcept>`. Omit the client certificate/key calls if mutual TLS is
not required. `loadPublicKeyFromPEMFile` is the existing API used to load the
client certificate. The same socket API exposes cipher selection, protocol
versions and PSK configuration. `configureTLS` is not invoked for plain HTTP or
for injected connections. Exceptions from it propagate to the caller.

## Inject an existing connection

`request.connection` accepts a shared pointer to an already connected
`Mantids30::Network::Sockets::Socket_Stream`, including TCP or TLS sockets.
For HTTPS, finish the TLS handshake before calling `perform()`.

```cpp
request.connection = connectedSocket;
request.responseContent = content;
auto response = client.perform(request);
```

| Operation | Automatic connection | Injected connection |
| --- | --- | --- |
| Resolve/connect to URL host | Yes | No |
| Set socket timeouts from Config | Yes | No |
| Configure/handshake TLS | Yes for HTTPS | No |
| Default Connection header | `close` | `keep-alive` |
| Close on success, failure or exception | Yes | No |

The URL still supplies Host and the request target. Its scheme must match the
socket: `https` needs a secure socket and `http` a plain socket. The caller is
responsible for connecting to and authenticating the intended server, including
hostname verification for TLS. A URL hostname different from the physical
endpoint can be useful for a deliberately configured virtual host or tunnel;
passing a URL does not authenticate an injected connection.

The client retains shared references while performing the request, and the
returned protocol object can also retain the socket object. You control explicit
closure of an injected socket; dropping one shared pointer is not an explicit
close operation.

## Reuse a connection for multiple requests

Create/inject one connection and issue sequential requests on it. This complete
example uses HTTPS with certificate and hostname validation:

```cpp
#include <Mantids30/Client/HTTPv1/client.h>
#include <Mantids30/Core/Memory/b_chunks.h>
#include <iostream>

int main()
{
    using namespace Mantids30;

    auto socket = std::make_shared<Network::Sockets::Socket_TLS>();
    socket->tlsKeys.setUseSystemCertificates(true);
    socket->tlsKeys.setValidateServerHostname(true);
    socket->overrideReadTimeout(30);
    socket->overrideWriteTimeout(30);
    if (!socket->connectTo("example.com", 443, 10))
    {
        return 1;
    }

    Client::HTTPv1 client;
    Client::HTTPv1::Request request;
    request.connection = socket;
    request.url = "https://example.com/first";
    request.responseContent = std::make_shared<Core::Memory::Containers::B_Chunks>();
    auto first = client.perform(request);
    if (!first.succeed())
    {
        socket->closeSocket();
        return 1;
    }

    // The server must have kept the connection open for the next request.
    request.url = "https://example.com/second";
    request.responseContent = std::make_shared<Core::Memory::Containers::B_Chunks>();
    auto second = client.perform(request);
    socket->closeSocket();

    if (!second.succeed())
    {
        std::cerr << second.errorMessage << '\n';
        return 1;
    }
    return 0;
}
```

Reuse requires a complete prior response, explicit response framing
(Content-Length, chunked or a response without a body), and server cooperation.
A connection-close response necessarily ends the connection. A server may close
an idle connection between calls; the client does not automatically reconnect or
retry an injected request. Inspect the server's Connection header and handle
transport errors on subsequent requests.

Use a new response destination each time. With no explicit `responseContent`,
each `perform()` creates a fresh default container automatically. Do not
pipeline requests or concurrently issue requests on one socket. After
cancellation, parse failure or a destination error, close or resynchronize the
injected connection before reusing it: unread bytes may remain.

Setting `Connection: keep-alive` alone does not preserve automatically created
connections. Persistent use is explicit through `request.connection`.

## Read response metadata

```cpp
auto response = client.perform(request);
if (response.succeed())
{
    auto contentType = response.getHeader("Protocol::HTTP::Content-Type");
    auto location = response.getHeader("Location");
    auto etag = response.protocol->getResponseETag().toString();
    auto cookies = response.protocol->serverResponse.headers.getOptionsByName("Set-Cookie");
    auto session = response.protocol->serverResponse.cookies.getCookieValueByName("session");
}
```

The low-level `serverResponse` also exposes status text/version, parsed
cache-control, ETag, WWW-Authenticate realm and supported security headers.
Use `headers.getOptionsByName()` for repeated values; `getHeader()` returns
only the first value. Response trailers are consumed by the chunk parser but
are not exposed as stored response headers.

The response body remains raw with respect to content type. To parse JSON, use
the framework's JSON APIs on the received bytes or provide a compatible JSON
streamable destination yourself. Content-Type does not automatically replace
your chosen response destination.

## Errors and ownership

| `Client::HTTPv1::Error` | Meaning |
| --- | --- |
| `NONE` | Complete request/response exchange; inspect HTTP status separately. |
| `INVALID_REQUEST` | Invalid URL/method/header/configuration or a URL scheme incompatible with an injected socket. |
| `CONNECTION` | Automatic connection/TLS failure, or an inactive injected socket. |
| `SEND` | Failure initializing/transmitting the HTTP request. |
| `RECEIVE` | Invalid/truncated response, socket read failure, unsupported transfer framing or destination write/EOF failure. |
| `CONTENT_LIMIT` | Declared or received response body exceeds `maxResponseSize`. |
| `CANCELLED` | A reception callback returned false. |

An oversized individual chunk fails parsing as `RECEIVE`; exceeding the total
body limit is `CONTENT_LIMIT`. Early validation errors may leave `content` and
`protocol` null. An HTTP status may be unavailable or incomplete on other
failures. Inspect the error before assuming those objects contain a valid result.

The request supplies shared pointers; it does not transfer exclusive ownership
of upload sources, destinations or injected connections. A response retains its
content and low-level protocol metadata after `perform()` returns. The protocol
object can retain referenced sources, callbacks and the transport object;
ensure references captured by callbacks remain valid for their actual use.

Application exceptions from request/TLS configuration, callbacks or storage may
propagate rather than become an Error value. There is no automatic retry.
Only use a destination's content as a complete response after a successful
exchange; on error the destination can contain accepted partial bytes.

## Supported behavior and limits

- HTTP/1.1 requests over plain TCP or TLS, including caller-injected streams.
- Fixed-size and chunked uploads; raw, JSON, URL-encoded and multipart bodies.
- Content-Length, chunked and connection-close response bodies; interim 1xx
  responses before the final response; HEAD, 204 and 304 body rules.
- Chunk extensions and trailer consumption, with configurable reception limits.
- Blocking calls with optional synchronous callbacks and cancellation.
- Sequential persistent connections through explicit injection.

Redirects are returned for the caller to handle. Cookies and cache metadata are
parsed without an automatic cookie jar or response cache. Content-Encoding
such as gzip is preserved without automatic decompression. Transfer-Encoding
support is limited to the implemented chunked framing; other transfer codings
are rejected. HTTP/2, HTTP/3, connection pools, proxy negotiation, pipelining,
WebSocket upgrades and an Expect: 100-continue upload handshake are not provided
by this high-level request API. Receiving informational responses does not mean
uploads are deferred until a server sends 100 Continue.

The class retains no shared per-request connection/cookie state. Concurrent use
still requires independent request objects, streams, sockets and callback state.
Configuration callbacks shared between threads must provide their own
synchronization when needed.

## Run the integration tests

From the repository root:

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON
cmake --build build --target client_httpv1_test -j4
ctest --test-dir build --output-on-failure -R '^client_httpv1'
```

The C++ tests exercise reception/framing, supplied destinations, callbacks,
cancellation, errors, storage spillover, request bodies and injected TCP sockets.
The HTTPS test is registered when Python 3 and the OpenSSL executable are
available; it generates a temporary certificate and checks trust validation,
hostname validation and an injected TLS socket. Tests use local loopback servers
and do not require an external HTTP service.
