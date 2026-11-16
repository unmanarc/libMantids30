# Core Classes Reference

All public classes live in the `Mantids30` namespace and are split into independent
libraries (one per source directory). Headers are installed as
`<prefix>/include/Mantids30/<Group>/<Module>/<header>.h`, so consumers include them with the
installed prefix:

```cpp
#include <Mantids30/Protocol/HTTP/httpv1_server.h>
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
- [Protocol](#protocols)
- [Web servers](#web-servers)
- [Sessions and API](#sessions-and-api)
- [Security and data formats](#security-and-data-formats)
- [Scripting](#scripting)
- [Program lifecycle, logs and config](#program-lifecycle-logs-and-config)
- [Helpers](#helpers)

## Library map

| Component | Library | Purpose |
|-----------|---------|---------|
| `Core/Helpers` | `Mantids30_Core_Helpers` | Crypto, random, files, encoders, TOTP, process helpers |
| `DataFormat/JSON` | `Mantids30_DataFormat_JSON` | JSON parsing, serialization and streamables (boost::json) |
| `Core/Memory` | `Mantids30_Core_Memory` | Typed variables, binary containers, streamables, parser/subparser |
| `Core/Threads` | `Mantids30_Core_Threads` | Thread wrappers, thread pool, garbage collector, safe containers |
| `DB/Common` | `Mantids30_DB_Common` | Database abstraction (`SQLConnector`, `Query`, `Transaction`) |
| `DB/SQLite3` | `Mantids30_DB_SQLite3` | SQLite3 backend (optional) |
| `DB/MariaDB` | `Mantids30_DB_MariaDB` | MariaDB/MySQL backend (optional) |
| `DB/PostgreSQL` | `Mantids30_DB_PostgreSQL` | PostgreSQL backend (optional) |
| `Network/Sockets` | `Mantids30_Network_Sockets` | Socket, acceptor and stream building blocks |
| `Network/Chains` | `Mantids30_Network_Chains` | Layered protocol chains (TLS, AES, XOR) |
| `Network/Interfaces` | `Mantids30_Network_Interfaces` | Interface discovery and TAP configuration |
| `Protocol/HTTP` | `Mantids30_Protocol_HTTP` | HTTP/1.1 client/server and WebSocket framing |
| `Protocol/MIME` | `Mantids30_Protocol_MIME` | MIME multipart parsing |
| `Protocol/FastRPC1` | `Mantids30_Protocol_FastRPC1` | RPC protocol server |
| `Protocol/LineRecv` | `Mantids30_Protocol_LineRecv` | Line oriented protocol |
| `Protocol/APISync` | `Mantids30_Protocol_APISync` | Synchronous API protocol parameters |
| `Server/WebRESTFul` | `Mantids30_Server_WebRESTFul` | RESTful web server, proxy, sessions, endpoints and WebSockets |
| `DataFormat/JWT` | `Mantids30_DataFormat_JWT` | JWT creation, validation and revocation |
| `Scripting/JSONExprEval` | `Mantids30_Scripting_JSONExprEval` | JSON expression evaluator |
| `Scripting/MantidsLang` | `Mantids30_Scripting_MantidsLang` | MantidsLang interpreter |
| `Runtime/Logs` | `Mantids30_Runtime_Logs` | Application, web and RPC logs |
| `Runtime/Service` | `Mantids30_Runtime_Service` | Application lifecycle, arguments and config loader |
| `ConfigBuilder/DataFormat_JWT` / `ConfigBuilder/Runtime_Logs` / `ConfigBuilder/Server_WebRESTFul` | `Mantids30_ConfigBuilder_*` | Configuration builders |
| `Application/FastRPC1Client` | `Mantids30_Application_FastRPC1Client` | FastRPC1 client application framework |

## Database

Common interfaces use `Mantids30::DB::Common`. Backends use
`Mantids30::DB::SQLite3`, `Mantids30::DB::MariaDB` and `Mantids30::DB::PostgreSQL`,
matching their header directories.

| Class | Header | Description |
|-------|--------|-------------|
| `SQLConnector` | `DB/Common/sqlconnector.h` | Base connector: connect, disconnect, query creation |
| `Query` | `DB/Common/query.h` | Prepared statement execution and result access |
| `Transaction` | `DB/Common/transaction.h` | Transaction scope management |
| `DatabaseCredentials` | `DB/Common/databasecredentials.h` | Host, port, user, password, database |
| `SQLConnector_SQLite3` | `DB/SQLite3/sqlconnector_sqlite3.h` | SQLite3 connector (file or in-memory) |
| `Query_SQLite3` | `DB/SQLite3/query_sqlite3.h` | SQLite3 query handler |
| `SQLConnector_MariaDB` | `DB/MariaDB/sqlconnector_mariadb.h` | MariaDB/MySQL connector |
| `Query_MariaDB` | `DB/MariaDB/query_mariadb.h` | MariaDB/MySQL query handler |
| `SQLConnector_PostgreSQL` | `DB/PostgreSQL/sqlconnector_pgsql.h` | PostgreSQL connector |
| `Query_PostgreSQL` | `DB/PostgreSQL/query_pgsql.h` | PostgreSQL query handler |

## Memory

### Abstract variables - `Mantids30::Core::Memory::Abstract`

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

### Containers - `Mantids30::Core::Memory::Containers`

| Class | Header | Description |
|-------|--------|-------------|
| `B_Base` | `Core/Memory/b_base.h` | Binary container interface |
| `BinaryContainerChunk` | `Core/Memory/b_chunk.h` | Single chunk of binary data |
| `B_Chunks` | `Core/Memory/b_chunks.h` | Chunk list container |
| `B_MEM` | `Core/Memory/b_mem.h` | Contiguous memory container |
| `B_MMAP` | `Core/Memory/b_mmap.h` | Memory mapped file container |
| `B_Ref` | `Core/Memory/b_ref.h` | Referenced (non-owning) container |
| `FileMap` | `Core/Memory/filemap.h` | Memory mapped file helper |

### Streams - `Mantids30::Core::Memory::Streams`

| Class | Header | Description |
|-------|--------|-------------|
| `StreamableObject` | `Core/Memory/streamable_object.h` | Base streamable target (`WriteStatus` result) |
| `StreamableString` | `Core/Memory/streamable_string.h` | Writes into a `std::string` |
| `StreamableFile` | `Core/Memory/streamable_file.h` | Writes into a file |
| `Mantids30::DataFormat::JSON::StreamableJSON` | `DataFormat/JSON/streamable_json.h` | Writes into a JSON value |
| `StreamableProcess` | `Core/Memory/streamable_process.h` | Writes into a child process stdin |
| `StreamableNull` | `Core/Memory/streamable_null.h` | Discards data |
| `StreamableTransformer` | `Core/Memory/streamable_transformer.h` | Streaming transformation stage |
| `Parser` | `Core/Memory/parser.h` | Protocol lifecycle orchestration and subparser chaining |
| `SubParser` | `Core/Memory/subparser.h` | Atomic parsing engine with strategies |

See [PARSER.md](PARSER.md) for the parsing architecture.

## Threads

Namespaces: `Mantids30::Core::Threads`, `Mantids30::Core::Threads::Safe`, `Mantids30::Core::Threads::Pool`

| Class | Header | Description |
|-------|--------|-------------|
| `Threaded` | `Core/Threads/threaded.h` | Thread wrapper with start/stop lifecycle |
| `GarbageCollector` | `Core/Threads/garbagecollector.h` | Deferred deletion helper |
| `ThreadPool` | `Core/Threads/threadpool.h` | Fixed size worker pool |
| `Mantids30::Core::Threads::Safe::Map` | `Core/Threads/safe_map.h` | Mutex protected map |
| `Mantids30::Core::Threads::Safe::MapItem` | `Core/Threads/safe_mapitem.h` | Value wrapper used by `Mantids30::Core::Threads::Safe::Map` |
| `Mantids30::Core::Threads::Safe::Queue` | `Core/Threads/safe_queue.h` | Mutex protected queue |
| `Mantids30::Core::Threads::Safe::SharedQueue` | `Core/Threads/safe_sharedqueue.h` | Shared pointer queue |

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

Namespace: `Mantids30::Network::Chains`

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

## Protocol

### HTTP/1.1 and WebSocket

Namespace: `Mantids30::Protocol::HTTP` (and sub-namespaces)

| Class | Header | Description |
|-------|--------|-------------|
| `HTTPv1_Base` | `Protocol/HTTP/httpv1_base.h` | Shared request/response parsing base |
| `HTTPv1_Server` | `Protocol/HTTP/httpv1_server.h` | HTTP/1.1 server protocol handler |
| `HTTPv1_Client` | `Protocol/HTTP/httpv1_client.h` | HTTP/1.1 client protocol handler |
| `Content` | `Protocol/HTTP/common_content.h` | Request/response body handling |
| `ContentChunkedTransformer` | `Protocol/HTTP/common_content_chunked_subparser.h` | Chunked transfer decoder |
| `Date` | `Protocol/HTTP/common_date.h` | HTTP date parsing/formatting |
| `URLVars` / `URLVarContent` | `Protocol/HTTP/common_urlvars.h`, `common_urlvar_subparser.h` | URL encoded variables |
| `Version` | `Protocol/HTTP/common_version.h` | HTTP version value |
| `Status` | `Protocol/HTTP/rsp_status.h` | Response status codes |
| `Mantids30::Protocol::HTTP::Headers::CacheControl`, `Mantids30::Protocol::HTTP::Headers::Cookie`, `Mantids30::Protocol::HTTP::Headers::ETag` | `Protocol/HTTP/hdr_*.h` | Standard headers |
| `Mantids30::Protocol::HTTP::Headers::Security::HSTS`, `XFrameOptions`, `XSSProtection` | `Protocol/HTTP/hdr_sec_*.h` | Security headers |
| `Mantids30::Protocol::HTTP::Request::RequestLine`, `Mantids30::Protocol::HTTP::Request::Cookies_ClientSide` | `Protocol/HTTP/req_*.h` | Request line and cookies |
| `Mantids30::Protocol::HTTP::Response::Cookies_ServerSide` | `Protocol/HTTP/rsp_cookies.h` | Response cookies |
| `Mantids30::Protocol::HTTP::WebSocket::FrameHeader`, `Mantids30::Protocol::HTTP::WebSocket::FrameContent` | `Protocol/HTTP/websocket_frame*.h` | WebSocket framing |
| `Mantids30::Protocol::HTTP::Decoders::URL` / `Mantids30::Protocol::HTTP::Encoders::URL` | `Protocol/HTTP/streamdecoder_url.h`, `streamencoder_url.h` | URL encode/decode stream stages |
| `Mantids30::Protocol::HTTP::APIReturn` | `Protocol/HTTP/api_return.h` | Standard API result envelope |

### MIME, FastRPC, LineRecv and APISync

| Class | Header | Description |
|-------|--------|-------------|
| `Mantids30::Protocol::MIME::MIME_Message` | `Protocol/MIME/mime_message.h` | Complete MIME message |
| `Mantids30::Protocol::MIME::MIME_PartMessage` | `Protocol/MIME/mime_partmessage.h` | MIME part |
| `Mantids30::Protocol::MIME::MIME_Sub_Content`, `MIME_Sub_FirstBoundary`, `MIME_Sub_EndPBoundary`, `MIME_Sub_Header`, `MIME_HeaderOption` | `Protocol/MIME/mime_sub_*.h` | MIME sub-parsers |
| `Mantids30::Protocol::FastRPC1::FastRPC1` | `Protocol/FastRPC1/fastrpc.h` | FastRPC1 server |
| `Mantids30::Protocol::LineRecv::LineRecv` / `LineRecv_SubParser` | `Protocol/LineRecv/linerecv*.h` | Line oriented protocol |
| `Mantids30::Protocol::APISync::APISyncParameters` | `Protocol/APISync/apisync.h` | APISync protocol parameters |

## Web servers

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `APIServerCore` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_core.h` | Core HTTP server with protocol handlers |
| `APIServer_ClientHandler` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_clienthandler.h` | Per-connection HTTP handling |
| `APIServerConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_config.h` | Listener/TLS configuration |
| `HTMLIEngine` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/htmliengine.h` | HTML include engine |
| `APIProxyParameters` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiproxy.h` | Reverse proxy parameters |
| `ResourcesFilter` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/resourcesfilter.h` | Resource authorization filter |
| `Engine` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/engine.h` | RESTful API server engine |
| `ClientHandler` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/clienthandler.h` | RESTful client/session handler |

## Sessions and API

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Session` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | HTTP session with client details |
| `SessionInfo` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | Session metadata |
| `ClientDetails` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | Remote client information |
| `Session_Vars` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session_vars.h` | Session variable storage |
| `Mantids30::Server::WebRESTFul::RESTful::Endpoints`, `RESTfulAPIEndpointFullDefinition`, `RequestContext` | `Mantids30::Server::WebRESTFul::RESTful` | `Server/WebRESTFul/api_restful_endpoints.h` | RESTful endpoint registry |
| `OptionsHandlerConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/api_options_handler.h` | HTTP OPTIONS handling |
| `Endpoints_Options` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/endpoints_options.h` | Endpoint options |
| `Mantids30::Server::WebRESTFul::WebSocket::Config`, `WebSocketConnection`, `Endpoint`, `Endpoints` | `Mantids30::Server::WebRESTFul::WebSocket` | `Server/WebRESTFul/api_websocket_*.h` | WebSocket endpoints and connections |
| `Mantids30::Server::WebRESTFul::Security::ReceivedAuth`, `Mantids30::Server::WebRESTFul::Security::Configuration` | `Mantids30::Server::WebRESTFul::Security` | `Server/WebRESTFul/security.h` | Authorization chain configuration |

## Security and data formats

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JWT` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Token creation, validation and revocation |
| `JWT::Token` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Decoded token claims |
| `JWT::Cache` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Revocation cache |
| `Builder` | `Mantids30::ConfigBuilder::DataFormat_JWT` | `ConfigBuilder/DataFormat_JWT/config_builder.h` | JWT configuration builder |

## Scripting

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `JSONEval` | `Mantids30::Scripting::JSONExprEval` | `Scripting/JSONExprEval/jsoneval.h` | JSON expression evaluator |
| `AtomicExpression`, `AtomicExpressionSide` | `Mantids30::Scripting::JSONExprEval` | `Scripting/JSONExprEval/atomicexpression*.h` | Expression operands |
| `MantidsLang` | `Mantids30::Scripting::MantidsLang` | `Scripting/MantidsLang/mantidslang.h` | MantidsLang interpreter (streamable) |

## Program lifecycle, logs and config

| Class | Namespace | Header | Description |
|-------|-----------|--------|-------------|
| `Application` | `Mantids30::Runtime::Service` | `Runtime/Service/application.h` | Application lifecycle, daemonization, signals |
| `CommandLineOption`, `GlobalArguments` | `Mantids30::Runtime::Service::Arguments` | `Runtime/Service/globalarguments.h` | Command line parsing |
| `Loader` | `Mantids30::Runtime::Service::Config` | `Runtime/Service/program_config.h` | Configuration file loader |
| `ProgramValues`, `Author` | `Mantids30::Runtime::Service::Values` | `Runtime/Service/programvalues.h` | Program metadata |
| `AppLog`, `LogBase`, `WebLog`, `RPCLog` | `Mantids30::Runtime::Logs` | `Runtime/Logs/*.h` | Logging backends with rotation |
| `Builder` | `Mantids30::ConfigBuilder::Runtime_Logs` | `ConfigBuilder/Runtime_Logs/config_builder.h` | Logging configuration builder |
| `Builder` | `Mantids30::ConfigBuilder::Server_WebRESTFul` | `ConfigBuilder/Server_WebRESTFul/config_builder.h` | REST server configuration builder |
| `APIProxyConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiproxy_config.h` | Proxy configuration builder |

## Helpers

Namespace: `Mantids30::Core::Helpers` (unless stated otherwise)

| Class / API | Header | Description |
|-------------|--------|-------------|
| `Crypto` | `Core/Helpers/crypto.h` | SHA/HMAC hashing and AES helpers |
| `Random` | `Core/Helpers/random.h` | Random strings and numbers |
| `File` | `Core/Helpers/file.h` | File read/write utilities |
| `Encoders` | `Core/Helpers/encoders.h` | Base64, hex and URL encoding |
| `StringConversions` | `Core/Helpers/strconv.h` | String/number conversions |
| `JSONReader2` | `Core/Helpers/json.h` | JSON reader (`Mantids30::DataFormat::JSON`) |
| `TOTP` | `Core/Helpers/totp.h` | Time based one-time passwords (`Mantids30::Core::Helpers::OTP`) |
| `Mantids30::DataFormat::JSON::DataTables` | `DataFormat/JSON/datatables.h` | DataTables server-side helper |
| `Mem` | `Core/Helpers/mem.h` | Memory helpers (zeroing, comparison) |
| `OS` | `Core/Helpers/os.h` | OS and environment helpers |
| `AppExec`, `AppSpawn` | `Core/Helpers/appexec.h` | Process execution and spawning |
| `safe_cast_or`, `in_range_17` | `Core/Helpers/safeint.h` | Safe integral conversions |
| `contains` | `Core/Helpers/stdmap.h` | Map lookup helper |
| `CALLBACK`, `CALLBACK_B` | `Core/Helpers/callbacks.h` | Callback invocation macros |

> The class list above mirrors the current headers. If a class is renamed, update this
document in the same pull request.
