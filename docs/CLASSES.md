# Core Classes Reference

All public classes live in the `Mantids30` namespace and are split into independent
libraries (one per source directory). Headers are installed as
`<prefix>/include/Mantids30/<Component>/<header>.h`, so consumers include them with the
installed prefix:

```cpp
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Net_Sockets/socket_tcp.h>
#include <Mantids30/Helpers/random.h>
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
| `Helpers` | `Mantids30_Helpers` | Crypto, random, files, encoders, TOTP, process helpers |
| `DataFormat_JSON` | `Mantids30_DataFormat_JSON` | JSON parsing, serialization and streamables (boost::json) |
| `Memory` | `Mantids30_Memory` | Typed variables, binary containers, streamables, parser/subparser |
| `Threads` | `Mantids30_Threads` | Thread wrappers, thread pool, garbage collector, safe containers |
| `DB` | `Mantids30_DB` | Database abstraction (`SQLConnector`, `Query`, `Transaction`) |
| `DB_SQLite3` | `Mantids30_DB_SQLite3` | SQLite3 backend (optional) |
| `DB_MariaDB` | `Mantids30_DB_MariaDB` | MariaDB/MySQL backend (optional) |
| `DB_PostgreSQL` | `Mantids30_DB_PostgreSQL` | PostgreSQL backend (optional) |
| `Net_Sockets` | `Mantids30_Net_Sockets` | Socket, acceptor and stream building blocks |
| `Net_Chains` | `Mantids30_Net_Chains` | Layered protocol chains (TLS, AES, XOR) |
| `Net_Interfaces` | `Mantids30_Net_Interfaces` | Interface discovery and TAP configuration |
| `Protocol_HTTP` | `Mantids30_Protocol_HTTP` | HTTP/1.1 client/server and WebSocket framing |
| `Protocol_MIME` | `Mantids30_Protocol_MIME` | MIME multipart parsing |
| `Protocol_FastRPC1` | `Mantids30_Protocol_FastRPC1` | RPC protocol server |
| `Protocol_LineRecv` | `Mantids30_Protocol_LineRecv` | Line oriented protocol |
| `Protocol_APISync` | `Mantids30_Protocol_APISync` | Synchronous API protocol parameters |
| `Server_WebCore` | `Mantids30_Server_WebCore` | Core web server, proxy and resource filtering |
| `Server_RESTfulWebAPI` | `Mantids30_Server_RESTfulWebAPI` | RESTful JSON API server |
| `API_EndpointsAndSessions` | `Mantids30_API_EndpointsAndSessions` | Sessions, endpoints and WebSocket endpoints |
| `DataFormat_JWT` | `Mantids30_DataFormat_JWT` | JWT creation, validation and revocation |
| `Scripts_JSONExprEval` | `Mantids30_Scripts_JSONExprEval` | JSON expression evaluator |
| `Scripts_MantidsLang` | `Mantids30_Scripts_MantidsLang` | MantidsLang interpreter |
| `Program_Logs` | `Mantids30_Program_Logs` | Application, web and RPC logs |
| `Program_Service` | `Mantids30_Program_Service` | Application lifecycle, arguments and config loader |
| `Config_JWT` / `Config_Logs` / `Config_REST` | `Mantids30_Config_*` | Configuration builders |
| `App_FastRPC1Client` | `Mantids30_App_FastRPC1Client` | FastRPC1 client application framework |

## Database

Namespace: `Mantids30::Database`

| Class | Header | Description |
|-------|--------|-------------|
| `SQLConnector` | `DB/sqlconnector.h` | Base connector: connect, disconnect, query creation |
| `Query` | `DB/query.h` | Prepared statement execution and result access |
| `Transaction` | `DB/transaction.h` | Transaction scope management |
| `DatabaseCredentials` | `DB/databasecredentials.h` | Host, port, user, password, database |
| `SQLConnector_SQLite3` | `DB_SQLite3/sqlconnector_sqlite3.h` | SQLite3 connector (file or in-memory) |
| `Query_SQLite3` | `DB_SQLite3/query_sqlite3.h` | SQLite3 query handler |
| `SQLConnector_MariaDB` | `DB_MariaDB/sqlconnector_mariadb.h` | MariaDB/MySQL connector |
| `Query_MariaDB` | `DB_MariaDB/query_mariadb.h` | MariaDB/MySQL query handler |
| `SQLConnector_PostgreSQL` | `DB_PostgreSQL/sqlconnector_pgsql.h` | PostgreSQL connector |
| `Query_PostgreSQL` | `DB_PostgreSQL/query_pgsql.h` | PostgreSQL query handler |

## Memory

### Abstract variables - `Mantids30::Memory::Abstract`

| Class | Header | Description |
|-------|--------|-------------|
| `Var` | `Memory/a_var.h` | Base class for all typed variables |
| `BINARY` | `Memory/a_bin.h` | Binary blob |
| `BOOL` | `Memory/a_bool.h` | Boolean |
| `DATETIME` | `Memory/a_datetime.h` | Date and time |
| `DOUBLE` | `Memory/a_double.h` | Double precision float |
| `INT8` / `INT16` / `INT32` / `INT64` | `Memory/a_int*.h` | Signed integers |
| `UINT8` / `UINT16` / `UINT32` / `UINT64` | `Memory/a_uint*.h` | Unsigned integers |
| `IPV4` / `IPV6` | `Memory/a_ipv*.h` | IP addresses |
| `MACADDR` | `Memory/a_macaddr.h` | MAC address |
| `PTR` | `Memory/a_ptr.h` | Pointer value |
| `STRING` / `VARCHAR` | `Memory/a_string.h`, `Memory/a_varchar.h` | Strings |
| `STRINGLIST` | `Memory/a_stringlist.h` | List of strings |
| `VariableMap` | `Memory/a_map.h` | Map of typed variables |
| `Vars` | `Memory/vars.h` | Variable collection with JSON/string conversion |

`Memory/a_allvars.h` includes every abstract type.

### Containers - `Mantids30::Memory::Containers`

| Class | Header | Description |
|-------|--------|-------------|
| `B_Base` | `Memory/b_base.h` | Binary container interface |
| `BinaryContainerChunk` | `Memory/b_chunk.h` | Single chunk of binary data |
| `B_Chunks` | `Memory/b_chunks.h` | Chunk list container |
| `B_MEM` | `Memory/b_mem.h` | Contiguous memory container |
| `B_MMAP` | `Memory/b_mmap.h` | Memory mapped file container |
| `B_Ref` | `Memory/b_ref.h` | Referenced (non-owning) container |
| `FileMap` | `Memory/filemap.h` | Memory mapped file helper |

### Streams - `Mantids30::Memory::Streams`

| Class | Header | Description |
|-------|--------|-------------|
| `StreamableObject` | `Memory/streamable_object.h` | Base streamable target (`WriteStatus` result) |
| `StreamableString` | `Memory/streamable_string.h` | Writes into a `std::string` |
| `StreamableFile` | `Memory/streamable_file.h` | Writes into a file |
| `StreamableJSON` | `DataFormat_JSON/streamable_json.h` | Writes into a JSON value |
| `StreamableProcess` | `Memory/streamable_process.h` | Writes into a child process stdin |
| `StreamableNull` | `Memory/streamable_null.h` | Discards data |
| `StreamableTransformer` | `Memory/streamable_transformer.h` | Streaming transformation stage |
| `Parser` | `Memory/parser.h` | Protocol lifecycle orchestration and subparser chaining |
| `SubParser` | `Memory/subparser.h` | Atomic parsing engine with strategies |

See [PARSER.md](PARSER.md) for the parsing architecture.

## Threads

Namespaces: `Mantids30::Threads`, `Mantids30::Threads::Safe`, `Mantids30::Threads::Pool`

| Class | Header | Description |
|-------|--------|-------------|
| `Threaded` | `Threads/threaded.h` | Thread wrapper with start/stop lifecycle |
| `GarbageCollector` | `Threads/garbagecollector.h` | Deferred deletion helper |
| `ThreadPool` | `Threads/threadpool.h` | Fixed size worker pool |
| `Safe::Map` | `Threads/safe_map.h` | Mutex protected map |
| `Safe::MapItem` | `Threads/safe_mapitem.h` | Value wrapper used by `Safe::Map` |
| `Safe::Queue` | `Threads/safe_queue.h` | Mutex protected queue |
| `Safe::SharedQueue` | `Threads/safe_sharedqueue.h` | Shared pointer queue |

## Network - sockets

Namespace: `Mantids30::Network::Sockets`

| Class | Header | Description |
|-------|--------|-------------|
| `Socket` | `Net_Sockets/socket.h` | Common socket base |
| `Socket_Stream` | `Net_Sockets/socket_stream.h` | Stream socket base with reader/writer helpers |
| `Socket_Stream_Reader` / `Socket_Stream_Writer` | `Net_Sockets/socket_stream_reader.h`, `socket_stream_writer.h` | Directional stream wrappers |
| `Socket_Stream_Dummy` | `Net_Sockets/socket_stream_dummy.h` | Loopback stream for tests |
| `Socket_TCP` | `Net_Sockets/socket_tcp.h` | TCP client socket |
| `Socket_TLS` | `Net_Sockets/socket_tls.h` | TLS client socket |
| `Socket_UDP` | `Net_Sockets/socket_udp.h` | UDP datagram socket |
| `Socket_UNIX` | `Net_Sockets/socket_unix.h` | UNIX domain stream socket |
| `Socket_DatagramBase` | `Net_Sockets/socket_datagrambase.h` | Datagram base class |
| `Connector` | `Net_Sockets/connector.h` | Outbound connection helper |
| `Listener` | `Net_Sockets/listener.h` | Listening socket helper |
| `Callbacks_Socket_TCP_Client` / `_Server` | `Net_Sockets/callbacks_socket_tcp_*.h` | TCP event callbacks |
| `Callbacks_Socket_TLS` / `_Server` | `Net_Sockets/callbacks_socket_tls*.h` | TLS event callbacks |

### Acceptors - `Mantids30::Network::Sockets::Acceptors`

| Class | Header | Description |
|-------|--------|-------------|
| `StreamAcceptorThread` | `Net_Sockets/acceptor_thread.h` | One thread per accepted connection |
| `MultiThreaded` | `Net_Sockets/acceptor_multithreaded.h` | Multiple acceptor threads |
| `PoolThreaded` | `Net_Sockets/acceptor_poolthreaded.h` | Thread pool based acceptor |
| `StreamAcceptorThreadCallbacks`, `ThreadPoolCallbacks`, `MultiThreadCallbacks` | `Net_Sockets/acceptor_callbacks.h` | Acceptor hooks |

### Network streams - `Mantids30::Network::Sockets::NetStreams`

| Class | Header | Description |
|-------|--------|-------------|
| `Bridge` | `Net_Sockets/streams_bridge.h` | Copies data between two streams |
| `Bridge_Thread` | `Net_Sockets/streams_bridge_thread.h` | Threaded stream bridge |
| `BufferedReader` | `Net_Sockets/streams_bufferedreader.h` | Buffered stream reader |
| `CryptoChallenge` | `Net_Sockets/streams_cryptochallenge.h` | Challenge/response stream decorator |

## Network - chains

Namespace: `Mantids30::Network::Sockets::ChainProtocols`

| Class | Header | Description |
|-------|--------|-------------|
| `Socket_Chain` | `Net_Chains/socket_chain.h` | Stack of layered protocols over a socket |
| `Socket_Chain_EndPointBase` | `Net_Chains/socket_chain_endpointbase.h` | Endpoint of a chain |
| `Socket_Chain_ProtocolBase` | `Net_Chains/socket_chain_protocolbase.h` | Base for chainable protocols |
| `Socket_Chain_TLS` | `Net_Chains/socket_chain_tls.h` | TLS layer |
| `Socket_Chain_AES` | `Net_Chains/socket_chain_aes.h` | AES layer |
| `Socket_Chain_XOR` | `Net_Chains/socket_chain_xor.h` | XOR obfuscation layer |

## Network - interfaces

Namespace: `Mantids30::Network::Interfaces`

| Class | Header | Description |
|-------|--------|-------------|
| `NetworkInterfaceConfiguration` | `Net_Interfaces/netifconfig.h` | Address/netmask/gateway configuration of an interface |
| `VirtualNetworkInterface` | `Net_Interfaces/virtualnetworkinterface.h` | TAP/virtual interface management (GPLv2 on WIN32) |

## Protocols

### HTTP/1.1 and WebSocket

Namespace: `Mantids30::Network::Protocol::HTTP` (and sub-namespaces)

| Class | Header | Description |
|-------|--------|-------------|
| `HTTPv1_Base` | `Protocol_HTTP/httpv1_base.h` | Shared request/response parsing base |
| `HTTPv1_Server` | `Protocol_HTTP/httpv1_server.h` | HTTP/1.1 server protocol handler |
| `HTTPv1_Client` | `Protocol_HTTP/httpv1_client.h` | HTTP/1.1 client protocol handler |
| `Content` | `Protocol_HTTP/common_content.h` | Request/response body handling |
| `ContentChunkedTransformer` | `Protocol_HTTP/common_content_chunked_subparser.h` | Chunked transfer decoder |
| `Date` | `Protocol_HTTP/common_date.h` | HTTP date parsing/formatting |
| `URLVars` / `URLVarContent` | `Protocol_HTTP/common_urlvars.h`, `common_urlvar_subparser.h` | URL encoded variables |
| `Version` | `Protocol_HTTP/common_version.h` | HTTP version value |
| `Status` | `Protocol_HTTP/rsp_status.h` | Response status codes |
| `Headers::CacheControl`, `Headers::Cookie`, `Headers::ETag` | `Protocol_HTTP/hdr_*.h` | Standard headers |
| `Headers::Security::HSTS`, `XFrameOptions`, `XSSProtection` | `Protocol_HTTP/hdr_sec_*.h` | Security headers |
| `Request::RequestLine`, `Request::Cookies_ClientSide` | `Protocol_HTTP/req_*.h` | Request line and cookies |
| `Response::Cookies_ServerSide` | `Protocol_HTTP/rsp_cookies.h` | Response cookies |
| `WebSocket::FrameHeader`, `WebSocket::FrameContent` | `Protocol_HTTP/websocket_frame*.h` | WebSocket framing |
| `Memory::Streams::Decoders::URL` / `Encoders::URL` | `Protocol_HTTP/streamdecoder_url.h`, `streamencoder_url.h` | URL encode/decode stream stages |
| `API::APIReturn` | `Protocol_HTTP/api_return.h` | Standard API result envelope |

### MIME, FastRPC, LineRecv and APISync

| Class | Header | Description |
|-------|--------|-------------|
| `MIME::MIME_Message` | `Protocol_MIME/mime_message.h` | Complete MIME message |
| `MIME::MIME_PartMessage` | `Protocol_MIME/mime_partmessage.h` | MIME part |
| `MIME::MIME_Sub_Content`, `MIME_Sub_FirstBoundary`, `MIME_Sub_EndPBoundary`, `MIME_Sub_Header`, `MIME_HeaderOption` | `Protocol_MIME/mime_sub_*.h` | MIME sub-parsers |
| `FastRPC::FastRPC1` | `Protocol_FastRPC1/fastrpc.h` | FastRPC1 server |
| `Line2Line::LineRecv` / `LineRecv_SubParser` | `Protocol_LineRecv/linerecv*.h` | Line oriented protocol |
| `APISync::APISyncParameters` | `Protocol_APISync/apisync.h` | APISync protocol parameters |

## Web servers

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `APIServerCore` | `Mantids30::Network::Servers::Web` | `Server_WebCore/apiserver_core.h` | Core HTTP server with protocol handlers |
| `APIServer_ClientHandler` | `Mantids30::Network::Servers::Web` | `Server_WebCore/apiserver_clienthandler.h` | Per-connection HTTP handling |
| `APIServerConfig` | `Mantids30::Network::Servers::Web` | `Server_WebCore/apiserver_config.h` | Listener/TLS configuration |
| `HTMLIEngine` | `Mantids30::Network::Servers::Web` | `Server_WebCore/htmliengine.h` | HTML include engine |
| `APIProxyParameters` | `Mantids30::Network::Servers::Web` | `Server_WebCore/apiproxy.h` | Reverse proxy parameters |
| `ResourcesFilter` | `Mantids30::API::Web` | `Server_WebCore/resourcesfilter.h` | Resource authorization filter |
| `Engine` | `Mantids30::Network::Servers::RESTful` | `Server_RESTfulWebAPI/engine.h` | RESTful API server engine |
| `ClientHandler` | `Mantids30::Network::Servers::RESTful` | `Server_RESTfulWebAPI/clienthandler.h` | RESTful client/session handler |

## Sessions and API

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Session` | `Mantids30::Sessions` | `API_EndpointsAndSessions/session.h` | HTTP session with client details |
| `SessionInfo` | `Mantids30::Sessions` | `API_EndpointsAndSessions/session.h` | Session metadata |
| `ClientDetails` | `Mantids30::Sessions` | `API_EndpointsAndSessions/session.h` | Remote client information |
| `Session_Vars` | `Mantids30::Sessions` | `API_EndpointsAndSessions/session_vars.h` | Session variable storage |
| `RESTful::Endpoints`, `RESTfulAPIEndpointFullDefinition`, `RequestContext` | `Mantids30::API::RESTful` | `API_EndpointsAndSessions/api_restful_endpoints.h` | RESTful endpoint registry |
| `OptionsHandlerConfig` | `Mantids30::API` | `API_EndpointsAndSessions/api_options_handler.h` | HTTP OPTIONS handling |
| `Endpoints_Options` | `Mantids30::API` | `API_EndpointsAndSessions/endpoints_options.h` | Endpoint options |
| `WebSocket::Config`, `WebSocketConnection`, `Endpoint`, `Endpoints` | `Mantids30::API::WebSocket` / `Network::Servers::Web` | `API_EndpointsAndSessions/api_websocket_*.h` | WebSocket endpoints and connections |
| `Security::ReceivedAuth`, `Security::Configuration` | `Mantids30::API::Security` | `API_EndpointsAndSessions/security.h` | Authorization chain configuration |

## Security and data formats

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JWT` | `Mantids30::DataFormat` | `DataFormat_JWT/jwt.h` | Token creation, validation and revocation |
| `JWT::Token` | `Mantids30::DataFormat` | `DataFormat_JWT/jwt.h` | Decoded token claims |
| `JWT::Cache` | `Mantids30::DataFormat` | `DataFormat_JWT/jwt.h` | Revocation cache |
| `Config::JWT` | `Mantids30::Program::Config` | `Config_JWT/config_builder.h` | JWT configuration builder |

## Scripting

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JSONEval` | `Mantids30::Scripts::Expressions` | `Scripts_JSONExprEval/jsoneval.h` | JSON expression evaluator |
| `AtomicExpression`, `AtomicExpressionSide` | `Mantids30::Scripts::Expressions` | `Scripts_JSONExprEval/atomicexpression*.h` | Expression operands |
| `MantidsLang` | `Mantids30::Scripts` | `Scripts_MantidsLang/mantidslang.h` | MantidsLang interpreter (streamable) |

## Program lifecycle, logs and config

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Application` | `Mantids30::Program` | `Program_Service/application.h` | Application lifecycle, daemonization, signals |
| `CommandLineOption`, `GlobalArguments` | `Mantids30::Program::Arguments` | `Program_Service/globalarguments.h` | Command line parsing |
| `Loader` | `Mantids30::Program::Config` | `Program_Service/program_config.h` | Configuration file loader |
| `ProgramValues`, `Author` | `Mantids30::Program::Values` | `Program_Service/programvalues.h` | Program metadata |
| `AppLog`, `LogBase`, `WebLog`, `RPCLog` | `Mantids30::Program::Logs` | `Program_Logs/*.h` | Logging backends with rotation |
| `Config::Logs`, `Config::RESTful_Engine`, `Config::APIProxyConfig` | `Mantids30::Program::Config` | `Config_Logs/`, `Config_REST/`, `Server_WebCore/apiproxy_config.h` | Configuration builders |

## Helpers

Namespace: `Mantids30::Helpers` (unless stated otherwise)

| Class / API | Header | Description |
|-------------|--------|-------------|
| `Crypto` | `Helpers/crypto.h` | SHA/HMAC hashing and AES helpers |
| `Random` | `Helpers/random.h` | Random strings and numbers |
| `File` | `Helpers/file.h` | File read/write utilities |
| `Encoders` | `Helpers/encoders.h` | Base64, hex and URL encoding |
| `StringConversions` | `Helpers/strconv.h` | String/number conversions |
| `JSONReader2` | `Helpers/json.h` | JSON reader (`Mantids30::Helpers::JSON`) |
| `TOTP` | `Helpers/totp.h` | Time based one-time passwords (`Mantids30::Helpers::OTP`) |
| `DataTables` | `Helpers/datatables.h` | DataTables server-side helper |
| `Mem` | `Helpers/mem.h` | Memory helpers (zeroing, comparison) |
| `OS` | `Helpers/os.h` | OS and environment helpers |
| `AppExec`, `AppSpawn` | `Helpers/appexec.h` | Process execution and spawning |
| `safe_cast_or`, `in_range_17` | `Helpers/safeint.h` | Safe integral conversions |
| `contains` | `Helpers/stdmap.h` | Map lookup helper |
| `CALLBACK`, `CALLBACK_B` | `Helpers/callbacks.h` | Callback invocation macros |

> The class list above mirrors the current headers. If a class is renamed, update this
document in the same pull request.
