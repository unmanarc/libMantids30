# Core Classes Reference

All public classes live in the `Mantids30` namespace and are split into independent
libraries (one per source directory). Headers are installed as
`<prefix>/include/Mantids30/<Group>/<Module>/<header>.h`, so consumers include them with the
installed prefix:

```cpp
#include <Mantids30/Protocols/HTTP/httpv1_server.h>
#include <Mantids30/Network/Sockets/socket_tcp.h>
#include <Mantids30/Core/Helpers/random.h>
```

Each component exposes a pkg-config file named `Mantids30_<Component>.pc` and a library
named `libMantids30_<Component>`. The header column below is relative to
`include/Mantids30/`.

## Table of Contents

- [Library map](#library-map)
- [Database](#database)
- [Memory](#memory)
- [Threads](#threads)
- [Network - sockets](#network---sockets)
- [Network - chains](#network---chains)
- [Network - interfaces](#network---interfaces)
- [Protocols](#protocols)
- [Web servers](#web-servers)
- [Sessions and API](#sessions-and-api)
- [Security and data formats](#security-and-data-formats)
- [Scripting](#scripting)
- [Program lifecycle, logs and config](#program-lifecycle-logs-and-config)
- [Helpers](#helpers)

## Library map

| Component | Library | Purpose |
|-----------|---------|---------|
| `Core/Helpers` | `Mantids30_Helpers` | Crypto, random, files, encoders, TOTP, process helpers |
| `DataFormats/JSON` | `Mantids30_DataFormat_JSON` | JSON parsing, serialization and streamables (boost::json) |
| `Core/Memory` | `Mantids30_Memory` | Typed variables, binary containers, streamables, parser/subparser |
| `Core/Threads` | `Mantids30_Threads` | Thread wrappers, thread pool, garbage collector, safe containers |
| `DB/DB` | `Mantids30_DB` | Database abstraction (`SQLConnector`, `Query`, `Transaction`) |
| `DB/SQLite3` | `Mantids30_DB_SQLite3` | SQLite3 backend (optional) |
| `DB/MariaDB` | `Mantids30_DB_MariaDB` | MariaDB/MySQL backend (optional) |
| `DB/PostgreSQL` | `Mantids30_DB_PostgreSQL` | PostgreSQL backend (optional) |
| `Network/Sockets` | `Mantids30_Net_Sockets` | Socket, acceptor and stream building blocks |
| `Network/Chains` | `Mantids30_Net_Chains` | Layered protocol chains (TLS, AES, XOR) |
| `Network/Interfaces` | `Mantids30_Net_Interfaces` | Interface discovery and TAP configuration |
| `Protocols/HTTP` | `Mantids30_Protocol_HTTP` | HTTP/1.1 client/server and WebSocket framing |
| `Protocols/MIME` | `Mantids30_Protocol_MIME` | MIME multipart parsing |
| `Protocols/FastRPC1` | `Mantids30_Protocol_FastRPC1` | RPC protocol server |
| `Protocols/LineRecv` | `Mantids30_Protocol_LineRecv` | Line oriented protocol |
| `Protocols/APISync` | `Mantids30_Protocol_APISync` | Synchronous API protocol parameters |
| `Servers/RESTfulWebServer` | `Mantids30_Server_RESTfulWebServer` | RESTful web server, proxy, sessions, endpoints and WebSockets |
| `DataFormats/JWT` | `Mantids30_DataFormat_JWT` | JWT creation, validation and revocation |
| `Scripting/JSONExprEval` | `Mantids30_Scripts_JSONExprEval` | JSON expression evaluator |
| `Scripting/MantidsLang` | `Mantids30_Scripts_MantidsLang` | MantidsLang interpreter |
| `Runtime/Logs` | `Mantids30_Program_Logs` | Application, web and RPC logs |
| `Runtime/Service` | `Mantids30_Program_Service` | Application lifecycle, arguments and config loader |
| `Config/JWT` / `Config/Logs` / `Config/REST` | `Mantids30_Config_*` | Configuration builders |
| `Applications/FastRPC1Client` | `Mantids30_App_FastRPC1Client` | FastRPC1 client application framework |

## Database

Namespace: `Mantids30::Database`

| Class | Header | Description |
|-------|--------|-------------|
| `SQLConnector` | `DB/DB/sqlconnector.h` | Base connector: connect, disconnect, query creation |
| `Query` | `DB/DB/query.h` | Prepared statement execution and result access |
| `Transaction` | `DB/DB/transaction.h` | Transaction scope management |
| `DatabaseCredentials` | `DB/DB/databasecredentials.h` | Host, port, user, password, database |
| `SQLConnector_SQLite3` | `DB/SQLite3/sqlconnector_sqlite3.h` | SQLite3 connector (file or in-memory) |
| `Query_SQLite3` | `DB/SQLite3/query_sqlite3.h` | SQLite3 query handler |
| `SQLConnector_MariaDB` | `DB/MariaDB/sqlconnector_mariadb.h` | MariaDB/MySQL connector |
| `Query_MariaDB` | `DB/MariaDB/query_mariadb.h` | MariaDB/MySQL query handler |
| `SQLConnector_PostgreSQL` | `DB/PostgreSQL/sqlconnector_pgsql.h` | PostgreSQL connector |
| `Query_PostgreSQL` | `DB/PostgreSQL/query_pgsql.h` | PostgreSQL query handler |

## Memory

### Abstract variables - `Mantids30::Memory::Abstract`

| Class | Header | Description |
|-------|--------|-------------|
| `Var` | `Core/Memory/a_var.h` | Base class for all typed variables |
| `BINARY` | `Core/Memory/a_bin.h` | Binary blob |
| `BOOL` | `Core/Memory/a_bool.h` | Boolean |
| `DATETIME` | `Core/Memory/a_datetime.h` | Date and time |
| `DOUBLE` | `Core/Memory/a_double.h` | Double precision float |
| `INT8` / `INT16` / `INT32` / `INT64` | `Core/Memory/a_int*.h` | Signed integers |
| `UINT8` / `UINT16` / `UINT32` / `UINT64` | `Core/Memory/a_uint*.h` | Unsigned integers |
| `IPV4` / `IPV6` | `Core/Memory/a_ipv*.h` | IP addresses |
| `MACADDR` | `Core/Memory/a_macaddr.h` | MAC address |
| `PTR` | `Core/Memory/a_ptr.h` | Pointer value |
| `STRING` / `VARCHAR` | `Core/Memory/a_string.h`, `Core/Memory/a_varchar.h` | Strings |
| `STRINGLIST` | `Core/Memory/a_stringlist.h` | List of strings |
| `VariableMap` | `Core/Memory/a_map.h` | Map of typed variables |
| `Vars` | `Core/Memory/vars.h` | Variable collection with JSON/string conversion |

`Core/Memory/a_allvars.h` includes every abstract type.

### Containers - `Mantids30::Memory::Containers`

| Class | Header | Description |
|-------|--------|-------------|
| `B_Base` | `Core/Memory/b_base.h` | Binary container interface |
| `BinaryContainerChunk` | `Core/Memory/b_chunk.h` | Single chunk of binary data |
| `B_Chunks` | `Core/Memory/b_chunks.h` | Chunk list container |
| `B_MEM` | `Core/Memory/b_mem.h` | Contiguous memory container |
| `B_MMAP` | `Core/Memory/b_mmap.h` | Memory mapped file container |
| `B_Ref` | `Core/Memory/b_ref.h` | Referenced (non-owning) container |
| `FileMap` | `Core/Memory/filemap.h` | Memory mapped file helper |

### Streams - `Mantids30::Memory::Streams`

| Class | Header | Description |
|-------|--------|-------------|
| `StreamableObject` | `Core/Memory/streamable_object.h` | Base streamable target (`WriteStatus` result) |
| `StreamableString` | `Core/Memory/streamable_string.h` | Writes into a `std::string` |
| `StreamableFile` | `Core/Memory/streamable_file.h` | Writes into a file |
| `StreamableJSON` | `DataFormats/JSON/streamable_json.h` | Writes into a JSON value |
| `StreamableProcess` | `Core/Memory/streamable_process.h` | Writes into a child process stdin |
| `StreamableNull` | `Core/Memory/streamable_null.h` | Discards data |
| `StreamableTransformer` | `Core/Memory/streamable_transformer.h` | Streaming transformation stage |
| `Parser` | `Core/Memory/parser.h` | Protocol lifecycle orchestration and subparser chaining |
| `SubParser` | `Core/Memory/subparser.h` | Atomic parsing engine with strategies |

See [PARSER.md](PARSER.md) for the parsing architecture.

## Threads

Namespaces: `Mantids30::Threads`, `Mantids30::Threads::Safe`, `Mantids30::Threads::Pool`

| Class | Header | Description |
|-------|--------|-------------|
| `Threaded` | `Core/Threads/threaded.h` | Thread wrapper with start/stop lifecycle |
| `GarbageCollector` | `Core/Threads/garbagecollector.h` | Deferred deletion helper |
| `ThreadPool` | `Core/Threads/threadpool.h` | Fixed size worker pool |
| `Safe::Map` | `Core/Threads/safe_map.h` | Mutex protected map |
| `Safe::MapItem` | `Core/Threads/safe_mapitem.h` | Value wrapper used by `Safe::Map` |
| `Safe::Queue` | `Core/Threads/safe_queue.h` | Mutex protected queue |
| `Safe::SharedQueue` | `Core/Threads/safe_sharedqueue.h` | Shared pointer queue |

## Network - sockets

Namespace: `Mantids30::Network::Sockets`

| Class | Header | Description |
|-------|--------|-------------|
| `Socket` | `Network/Sockets/socket.h` | Common socket base |
| `Socket_Stream` | `Network/Sockets/socket_stream.h` | Stream socket base with reader/writer helpers |
| `Socket_Stream_Reader` / `Socket_Stream_Writer` | `Network/Sockets/socket_stream_reader.h`, `socket_stream_writer.h` | Directional stream wrappers |
| `Socket_Stream_Dummy` | `Network/Sockets/socket_stream_dummy.h` | Loopback stream for tests |
| `Socket_TCP` | `Network/Sockets/socket_tcp.h` | TCP client socket |
| `Socket_TLS` | `Network/Sockets/socket_tls.h` | TLS client socket |
| `Socket_UDP` | `Network/Sockets/socket_udp.h` | UDP datagram socket |
| `Socket_UNIX` | `Network/Sockets/socket_unix.h` | UNIX domain stream socket |
| `Socket_DatagramBase` | `Network/Sockets/socket_datagrambase.h` | Datagram base class |
| `Connector` | `Network/Sockets/connector.h` | Outbound connection helper |
| `Listener` | `Network/Sockets/listener.h` | Listening socket helper |
| `Callbacks_Socket_TCP_Client` / `_Server` | `Network/Sockets/callbacks_socket_tcp_*.h` | TCP event callbacks |
| `Callbacks_Socket_TLS` / `_Server` | `Network/Sockets/callbacks_socket_tls*.h` | TLS event callbacks |

### Acceptors - `Mantids30::Network::Sockets::Acceptors`

| Class | Header | Description |
|-------|--------|-------------|
| `StreamAcceptorThread` | `Network/Sockets/acceptor_thread.h` | One thread per accepted connection |
| `MultiThreaded` | `Network/Sockets/acceptor_multithreaded.h` | Multiple acceptor threads |
| `PoolThreaded` | `Network/Sockets/acceptor_poolthreaded.h` | Thread pool based acceptor |
| `StreamAcceptorThreadCallbacks`, `ThreadPoolCallbacks`, `MultiThreadCallbacks` | `Network/Sockets/acceptor_callbacks.h` | Acceptor hooks |

### Network streams - `Mantids30::Network::Sockets::NetStreams`

| Class | Header | Description |
|-------|--------|-------------|
| `Bridge` | `Network/Sockets/streams_bridge.h` | Copies data between two streams |
| `Bridge_Thread` | `Network/Sockets/streams_bridge_thread.h` | Threaded stream bridge |
| `BufferedReader` | `Network/Sockets/streams_bufferedreader.h` | Buffered stream reader |
| `CryptoChallenge` | `Network/Sockets/streams_cryptochallenge.h` | Challenge/response stream decorator |

## Network - chains

Namespace: `Mantids30::Network::Sockets::ChainProtocols`

| Class | Header | Description |
|-------|--------|-------------|
| `Socket_Chain` | `Network/Chains/socket_chain.h` | Stack of layered protocols over a socket |
| `Socket_Chain_EndPointBase` | `Network/Chains/socket_chain_endpointbase.h` | Endpoint of a chain |
| `Socket_Chain_ProtocolBase` | `Network/Chains/socket_chain_protocolbase.h` | Base for chainable protocols |
| `Socket_Chain_TLS` | `Network/Chains/socket_chain_tls.h` | TLS layer |
| `Socket_Chain_AES` | `Network/Chains/socket_chain_aes.h` | AES layer |
| `Socket_Chain_XOR` | `Network/Chains/socket_chain_xor.h` | XOR obfuscation layer |

## Network - interfaces

Namespace: `Mantids30::Network::Interfaces`

| Class | Header | Description |
|-------|--------|-------------|
| `NetworkInterfaceConfiguration` | `Network/Interfaces/netifconfig.h` | Address/netmask/gateway configuration of an interface |
| `VirtualNetworkInterface` | `Network/Interfaces/virtualnetworkinterface.h` | TAP/virtual interface management (GPLv2 on WIN32) |

## Protocols

### HTTP/1.1 and WebSocket

Namespace: `Mantids30::Network::Protocol::HTTP` (and sub-namespaces)

| Class | Header | Description |
|-------|--------|-------------|
| `HTTPv1_Base` | `Protocols/HTTP/httpv1_base.h` | Shared request/response parsing base |
| `HTTPv1_Server` | `Protocols/HTTP/httpv1_server.h` | HTTP/1.1 server protocol handler |
| `HTTPv1_Client` | `Protocols/HTTP/httpv1_client.h` | HTTP/1.1 client protocol handler |
| `Content` | `Protocols/HTTP/common_content.h` | Request/response body handling |
| `ContentChunkedTransformer` | `Protocols/HTTP/common_content_chunked_subparser.h` | Chunked transfer decoder |
| `Date` | `Protocols/HTTP/common_date.h` | HTTP date parsing/formatting |
| `URLVars` / `URLVarContent` | `Protocols/HTTP/common_urlvars.h`, `common_urlvar_subparser.h` | URL encoded variables |
| `Version` | `Protocols/HTTP/common_version.h` | HTTP version value |
| `Status` | `Protocols/HTTP/rsp_status.h` | Response status codes |
| `Headers::CacheControl`, `Headers::Cookie`, `Headers::ETag` | `Protocols/HTTP/hdr_*.h` | Standard headers |
| `Headers::Security::HSTS`, `XFrameOptions`, `XSSProtection` | `Protocols/HTTP/hdr_sec_*.h` | Security headers |
| `Request::RequestLine`, `Request::Cookies_ClientSide` | `Protocols/HTTP/req_*.h` | Request line and cookies |
| `Response::Cookies_ServerSide` | `Protocols/HTTP/rsp_cookies.h` | Response cookies |
| `WebSocket::FrameHeader`, `WebSocket::FrameContent` | `Protocols/HTTP/websocket_frame*.h` | WebSocket framing |
| `Memory::Streams::Decoders::URL` / `Encoders::URL` | `Protocols/HTTP/streamdecoder_url.h`, `streamencoder_url.h` | URL encode/decode stream stages |
| `API::APIReturn` | `Protocols/HTTP/api_return.h` | Standard API result envelope |

### MIME, FastRPC, LineRecv and APISync

| Class | Header | Description |
|-------|--------|-------------|
| `MIME::MIME_Message` | `Protocols/MIME/mime_message.h` | Complete MIME message |
| `MIME::MIME_PartMessage` | `Protocols/MIME/mime_partmessage.h` | MIME part |
| `MIME::MIME_Sub_Content`, `MIME_Sub_FirstBoundary`, `MIME_Sub_EndPBoundary`, `MIME_Sub_Header`, `MIME_HeaderOption` | `Protocols/MIME/mime_sub_*.h` | MIME sub-parsers |
| `FastRPC::FastRPC1` | `Protocols/FastRPC1/fastrpc.h` | FastRPC1 server |
| `Line2Line::LineRecv` / `LineRecv_SubParser` | `Protocols/LineRecv/linerecv*.h` | Line oriented protocol |
| `APISync::APISyncParameters` | `Protocols/APISync/apisync.h` | APISync protocol parameters |

## Web servers

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `APIServerCore` | `Mantids30::Network::Servers::Web` | `Servers/RESTfulWebServer/apiserver_core.h` | Core HTTP server with protocol handlers |
| `APIServer_ClientHandler` | `Mantids30::Network::Servers::Web` | `Servers/RESTfulWebServer/apiserver_clienthandler.h` | Per-connection HTTP handling |
| `APIServerConfig` | `Mantids30::Network::Servers::Web` | `Servers/RESTfulWebServer/apiserver_config.h` | Listener/TLS configuration |
| `HTMLIEngine` | `Mantids30::Network::Servers::Web` | `Servers/RESTfulWebServer/htmliengine.h` | HTML include engine |
| `APIProxyParameters` | `Mantids30::Network::Servers::Web` | `Servers/RESTfulWebServer/apiproxy.h` | Reverse proxy parameters |
| `ResourcesFilter` | `Mantids30::API::Web` | `Servers/RESTfulWebServer/resourcesfilter.h` | Resource authorization filter |
| `Engine` | `Mantids30::Network::Servers::RESTful` | `Servers/RESTfulWebServer/engine.h` | RESTful API server engine |
| `ClientHandler` | `Mantids30::Network::Servers::RESTful` | `Servers/RESTfulWebServer/clienthandler.h` | RESTful client/session handler |

## Sessions and API

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Session` | `Mantids30::Sessions` | `Servers/RESTfulWebServer/session.h` | HTTP session with client details |
| `SessionInfo` | `Mantids30::Sessions` | `Servers/RESTfulWebServer/session.h` | Session metadata |
| `ClientDetails` | `Mantids30::Sessions` | `Servers/RESTfulWebServer/session.h` | Remote client information |
| `Session_Vars` | `Mantids30::Sessions` | `Servers/RESTfulWebServer/session_vars.h` | Session variable storage |
| `RESTful::Endpoints`, `RESTfulAPIEndpointFullDefinition`, `RequestContext` | `Mantids30::API::RESTful` | `Servers/RESTfulWebServer/api_restful_endpoints.h` | RESTful endpoint registry |
| `OptionsHandlerConfig` | `Mantids30::API` | `Servers/RESTfulWebServer/api_options_handler.h` | HTTP OPTIONS handling |
| `Endpoints_Options` | `Mantids30::API` | `Servers/RESTfulWebServer/endpoints_options.h` | Endpoint options |
| `WebSocket::Config`, `WebSocketConnection`, `Endpoint`, `Endpoints` | `Mantids30::API::WebSocket` / `Network::Servers::Web` | `Servers/RESTfulWebServer/api_websocket_*.h` | WebSocket endpoints and connections |
| `Security::ReceivedAuth`, `Security::Configuration` | `Mantids30::API::Security` | `Servers/RESTfulWebServer/security.h` | Authorization chain configuration |

## Security and data formats

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JWT` | `Mantids30::DataFormat` | `DataFormats/JWT/jwt.h` | Token creation, validation and revocation |
| `JWT::Token` | `Mantids30::DataFormat` | `DataFormats/JWT/jwt.h` | Decoded token claims |
| `JWT::Cache` | `Mantids30::DataFormat` | `DataFormats/JWT/jwt.h` | Revocation cache |
| `Config::JWT` | `Mantids30::Program::Config` | `Config/JWT/config_builder.h` | JWT configuration builder |

## Scripting

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JSONEval` | `Mantids30::Scripts::Expressions` | `Scripting/JSONExprEval/jsoneval.h` | JSON expression evaluator |
| `AtomicExpression`, `AtomicExpressionSide` | `Mantids30::Scripts::Expressions` | `Scripting/JSONExprEval/atomicexpression*.h` | Expression operands |
| `MantidsLang` | `Mantids30::Scripts` | `Scripting/MantidsLang/mantidslang.h` | MantidsLang interpreter (streamable) |

## Program lifecycle, logs and config

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Application` | `Mantids30::Program` | `Runtime/Service/application.h` | Application lifecycle, daemonization, signals |
| `CommandLineOption`, `GlobalArguments` | `Mantids30::Program::Arguments` | `Runtime/Service/globalarguments.h` | Command line parsing |
| `Loader` | `Mantids30::Program::Config` | `Runtime/Service/program_config.h` | Configuration file loader |
| `ProgramValues`, `Author` | `Mantids30::Program::Values` | `Runtime/Service/programvalues.h` | Program metadata |
| `AppLog`, `LogBase`, `WebLog`, `RPCLog` | `Mantids30::Program::Logs` | `Runtime/Logs/*.h` | Logging backends with rotation |
| `Config::Logs`, `Config::RESTful_Engine`, `Config::APIProxyConfig` | `Mantids30::Program::Config` | `Config/Logs/`, `Config/REST/`, `Servers/RESTfulWebServer/apiproxy_config.h` | Configuration builders |

## Helpers

Namespace: `Mantids30::Helpers` (unless stated otherwise)

| Class / API | Header | Description |
|-------------|--------|-------------|
| `Crypto` | `Core/Helpers/crypto.h` | SHA/HMAC hashing and AES helpers |
| `Random` | `Core/Helpers/random.h` | Random strings and numbers |
| `File` | `Core/Helpers/file.h` | File read/write utilities |
| `Encoders` | `Core/Helpers/encoders.h` | Base64, hex and URL encoding |
| `StringConversions` | `Core/Helpers/strconv.h` | String/number conversions |
| `JSONReader2` | `Core/Helpers/json.h` | JSON reader (`Mantids30::Helpers::JSON`) |
| `TOTP` | `Core/Helpers/totp.h` | Time based one-time passwords (`Mantids30::Helpers::OTP`) |
| `DataTables` | `Core/Helpers/datatables.h` | DataTables server-side helper |
| `Mem` | `Core/Helpers/mem.h` | Memory helpers (zeroing, comparison) |
| `OS` | `Core/Helpers/os.h` | OS and environment helpers |
| `AppExec`, `AppSpawn` | `Core/Helpers/appexec.h` | Process execution and spawning |
| `safe_cast_or`, `in_range_17` | `Core/Helpers/safeint.h` | Safe integral conversions |
| `contains` | `Core/Helpers/stdmap.h` | Map lookup helper |
| `CALLBACK`, `CALLBACK_B` | `Core/Helpers/callbacks.h` | Callback invocation macros |

> The class list above mirrors the current headers. If a class is renamed, update this
document in the same pull request.
