# Classes and APIs Reference

Public classes and APIs are organized by source group and module under `Mantids30/`.
The library map and reference sections follow the same order.

Headers are installed as `<prefix>/include/Mantids30/<Group>/<Module>/<header>.h`:

```cpp
#include <Mantids30/Core/Helpers/random.h>
#include <Mantids30/Network/Sockets/socket_tcp.h>
#include <Mantids30/Client/HTTPv1/client.h>
```

Each module exposes a library named `libMantids30_<Group>_<Module>` and a
pkg-config file named `Mantids30_<Group>_<Module>.pc`. Header paths in the tables
are relative to `include/Mantids30/`. Type names are relative to the namespace
column unless fully qualified. A dash marks macros; `::` marks the global namespace.

## Table of Contents

- [Library map](#library-map)
- [Core](#core)
  - [Core/Helpers](#corehelpers)
  - [Core/Memory](#corememory)
  - [Core/Threads](#corethreads)
- [DataFormat](#dataformat)
  - [DataFormat/JSON](#dataformatjson)
  - [DataFormat/JWT](#dataformatjwt)
- [DB](#db)
  - [DB/Common](#dbcommon)
  - [DB/MariaDB](#dbmariadb)
  - [DB/PostgreSQL](#dbpostgresql)
  - [DB/SQLite3](#dbsqlite3)
- [Network](#network)
  - [Network/Chains](#networkchains)
  - [Network/Interfaces](#networkinterfaces)
  - [Network/Sockets](#networksockets)
- [Protocol](#protocol)
  - [Protocol/FastRPC1](#protocolfastrpc1)
  - [Protocol/HTTP](#protocolhttp)
  - [Protocol/LineRecv](#protocollinerecv)
  - [Protocol/MIME](#protocolmime)
- [Client](#client)
  - [Client/APISync](#clientapisync)
  - [Client/HTTPv1](#clienthttpv1)
- [Server](#server)
  - [Server/WebRESTFul](#serverwebrestful)
- [Scripting](#scripting)
  - [Scripting/JSONExprEval](#scriptingjsonexpreval)
  - [Scripting/MantidsLang](#scriptingmantidslang)
- [Runtime](#runtime)
  - [Runtime/Logs](#runtimelogs)
  - [Runtime/Service](#runtimeservice)
- [ConfigBuilder](#configbuilder)
  - [ConfigBuilder/DataFormat_JWT](#configbuilderdataformat_jwt)
  - [ConfigBuilder/Runtime_Logs](#configbuilderruntime_logs)
  - [ConfigBuilder/Server_WebRESTFul](#configbuilderserver_webrestful)
- [Application](#application)
  - [Application/FastRPC1Client](#applicationfastrpc1client)

## Library map

| Component | Library | Purpose |
|-----------|---------|---------|
| `Core/Helpers` | `Mantids30_Core_Helpers` | Crypto, random, files, encoders, TOTP, process helpers |
| `Core/Memory` | `Mantids30_Core_Memory` | Typed variables, binary containers, streamables, parser/subparser |
| `Core/Threads` | `Mantids30_Core_Threads` | Thread wrappers, thread pool, garbage collector, safe containers |
| `DataFormat/JSON` | `Mantids30_DataFormat_JSON` | JSON parsing, serialization and streamables (boost::json) |
| `DataFormat/JWT` | `Mantids30_DataFormat_JWT` | JWT creation, validation and revocation |
| `DB/Common` | `Mantids30_DB_Common` | Database abstraction (`SQLConnector`, `Query`, `Transaction`) |
| `DB/MariaDB` | `Mantids30_DB_MariaDB` | MariaDB/MySQL backend (optional) |
| `DB/PostgreSQL` | `Mantids30_DB_PostgreSQL` | PostgreSQL backend (optional) |
| `DB/SQLite3` | `Mantids30_DB_SQLite3` | SQLite3 backend (optional) |
| `Network/Chains` | `Mantids30_Network_Chains` | Layered protocol chains (TLS, AES, XOR) |
| `Network/Interfaces` | `Mantids30_Network_Interfaces` | Interface discovery and TAP configuration |
| `Network/Sockets` | `Mantids30_Network_Sockets` | Socket, acceptor and stream building blocks |
| `Protocol/FastRPC1` | `Mantids30_Protocol_FastRPC1` | RPC protocol server |
| `Protocol/HTTP` | `Mantids30_Protocol_HTTP` | HTTP/1.1 client/server and WebSocket framing |
| `Protocol/LineRecv` | `Mantids30_Protocol_LineRecv` | Line oriented protocol |
| `Protocol/MIME` | `Mantids30_Protocol_MIME` | MIME multipart parsing |
| `Client/APISync` | `Mantids30_Client_APISync` | API synchronization client over HTTP/HTTPS |
| `Client/HTTPv1` | `Mantids30_Client_HTTPv1` | Synchronous HTTP client: streams, uploads, callbacks, TLS and persistent connections |
| `Server/WebRESTFul` | `Mantids30_Server_WebRESTFul` | RESTful web server, proxy, sessions, endpoints and WebSockets |
| `Scripting/JSONExprEval` | `Mantids30_Scripting_JSONExprEval` | JSON expression evaluator |
| `Scripting/MantidsLang` | `Mantids30_Scripting_MantidsLang` | MantidsLang interpreter |
| `Runtime/Logs` | `Mantids30_Runtime_Logs` | Application, web and RPC logs |
| `Runtime/Service` | `Mantids30_Runtime_Service` | Application lifecycle, arguments and config loader |
| `ConfigBuilder/DataFormat_JWT` | `Mantids30_ConfigBuilder_DataFormat_JWT` | Configuration builders |
| `ConfigBuilder/Runtime_Logs` | `Mantids30_ConfigBuilder_Runtime_Logs` | Configuration builders |
| `ConfigBuilder/Server_WebRESTFul` | `Mantids30_ConfigBuilder_Server_WebRESTFul` | Configuration builders |
| `Application/FastRPC1Client` | `Mantids30_Application_FastRPC1Client` | FastRPC1 client application framework |

## Core

### Core/Helpers

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Crypto` | `Mantids30::Core::Helpers` | `Core/Helpers/crypto.h` | SHA/HMAC hashing and AES helpers |
| `Random` | `Mantids30::Core::Helpers` | `Core/Helpers/random.h` | Random strings and numbers |
| `File` | `Mantids30::Core::Helpers` | `Core/Helpers/file.h` | File read/write utilities |
| `Encoders` | `Mantids30::Core::Helpers` | `Core/Helpers/encoders.h` | Base64, hex and URL encoding |
| `StringConversions` | `Mantids30::Core::Helpers` | `Core/Helpers/strconv.h` | String/number conversions |
| `TOTP` | `Mantids30::Core::Helpers::OTP` | `Core/Helpers/totp.h` | Time based one-time passwords |
| `Mem` | `Mantids30::Core::Helpers` | `Core/Helpers/mem.h` | Memory helpers (zeroing, comparison) |
| `OS` | `Mantids30::Core::Helpers` | `Core/Helpers/os.h` | OS and environment helpers |
| `AppExec`, `AppSpawn` | `Mantids30::Core::Helpers` | `Core/Helpers/appexec.h` | Process execution and spawning |
| `safe_cast_or`, `in_range_17` | `::` | `Core/Helpers/safeint.h` | Safe integral conversions |
| `contains` | `Mantids30::Core::Helpers` | `Core/Helpers/stdmap.h` | Map lookup helper |
| `CALLBACK`, `CALLBACK_B` | — | `Core/Helpers/callbacks.h` | Callback invocation macros |

### Core/Memory

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Var` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_var.h` | Base class for all typed variables |
| `BINARY` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_bin.h` | Binary blob |
| `BOOL` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_bool.h` | Boolean |
| `DATETIME` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_datetime.h` | Date and time |
| `DOUBLE` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_double.h` | Double precision float |
| `INT8` / `INT16` / `INT32` / `INT64` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_int*.h` | Signed integers |
| `UINT8` / `UINT16` / `UINT32` / `UINT64` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_uint*.h` | Unsigned integers |
| `IPV4` / `IPV6` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_ipv*.h` | IP addresses |
| `MACADDR` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_macaddr.h` | MAC address |
| `PTR` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_ptr.h` | Pointer value |
| `STRING` / `VARCHAR` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_string.h`, `Core/Memory/a_varchar.h` | Strings |
| `STRINGLIST` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_stringlist.h` | List of strings |
| `VariableMap` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/a_map.h` | Map of typed variables |
| `Vars` | `Mantids30::Core::Memory::Abstract` | `Core/Memory/vars.h` | Variable collection with JSON/string conversion |
| `B_Base` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_base.h` | Binary container interface |
| `BinaryContainerChunk` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_chunk.h` | Single chunk of binary data |
| `B_Chunks` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_chunks.h` | Chunk list container |
| `B_MEM` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_mem.h` | Contiguous memory container |
| `B_MMAP` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_mmap.h` | Memory mapped file container |
| `B_Ref` | `Mantids30::Core::Memory::Containers` | `Core/Memory/b_ref.h` | Referenced (non-owning) container |
| `FileMap` | `Mantids30::Core::Memory::Containers` | `Core/Memory/filemap.h` | Memory mapped file helper |
| `StreamableObject` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_object.h` | Base streamable target (`WriteStatus` result) |
| `StreamableString` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_string.h` | Writes into a `std::string` |
| `StreamableFile` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_file.h` | Writes into a file |
| `StreamableProcess` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_process.h` | Writes into a child process stdin |
| `StreamableNull` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_null.h` | Discards data |
| `StreamableTransformer` | `Mantids30::Core::Memory::Streams` | `Core/Memory/streamable_transformer.h` | Streaming transformation stage |
| `Parser` | `Mantids30::Core::Memory::Streams` | `Core/Memory/parser.h` | Protocol lifecycle orchestration and subparser chaining |
| `SubParser` | `Mantids30::Core::Memory::Streams` | `Core/Memory/subparser.h` | Atomic parsing engine with strategies |

`Core/Memory/a_allvars.h` includes every abstract variable type.
See [PARSER.md](PARSER.md) for the parsing architecture.

### Core/Threads

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Threaded` | `Mantids30::Core::Threads` | `Core/Threads/threaded.h` | Thread wrapper with start/stop lifecycle |
| `GarbageCollector` | `Mantids30::Core::Threads` | `Core/Threads/garbagecollector.h` | Deferred deletion helper |
| `ThreadPool` | `Mantids30::Core::Threads::Pool` | `Core/Threads/threadpool.h` | Fixed size worker pool |
| `Map` | `Mantids30::Core::Threads::Safe` | `Core/Threads/safe_map.h` | Mutex protected map |
| `MapItem` | `Mantids30::Core::Threads::Safe` | `Core/Threads/safe_mapitem.h` | Value wrapper used by `Mantids30::Core::Threads::Safe::Map` |
| `Queue` | `Mantids30::Core::Threads::Safe` | `Core/Threads/safe_queue.h` | Mutex protected queue |
| `SharedQueue` | `Mantids30::Core::Threads::Safe` | `Core/Threads/safe_sharedqueue.h` | Shared pointer queue |

## DataFormat

### DataFormat/JSON

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `StreamableJSON` | `Mantids30::DataFormat::JSON` | `DataFormat/JSON/streamable_json.h` | Writes into a JSON value |
| `Reader` | `Mantids30::DataFormat::JSON` | `DataFormat/JSON/json_convert.h` | JSON value reader and conversion helpers |
| `DataTables` | `Mantids30::DataFormat::JSON` | `DataFormat/JSON/datatables.h` | DataTables server-side helper |
| `Path` | `Mantids30::DataFormat::JSON` | `DataFormat/JSON/json_path.h` | JSON path traversal and value access |

### DataFormat/JWT

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `JWT` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Token creation, validation and revocation |
| `JWT::Token` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Decoded token claims |
| `JWT::Cache` | `Mantids30::DataFormat::JWT` | `DataFormat/JWT/jwt.h` | Revocation cache |

## DB

### DB/Common

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `SQLConnector` | `Mantids30::DB::Common` | `DB/Common/sqlconnector.h` | Base connector: connect, disconnect, query creation |
| `Query` | `Mantids30::DB::Common` | `DB/Common/query.h` | Prepared statement execution and result access |
| `Transaction` | `Mantids30::DB::Common` | `DB/Common/transaction.h` | Transaction scope management |
| `DatabaseCredentials` | `Mantids30::DB::Common` | `DB/Common/databasecredentials.h` | Host, port, user, password, database |

### DB/MariaDB

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `SQLConnector_MariaDB` | `Mantids30::DB::MariaDB` | `DB/MariaDB/sqlconnector_mariadb.h` | MariaDB/MySQL connector |
| `Query_MariaDB` | `Mantids30::DB::MariaDB` | `DB/MariaDB/query_mariadb.h` | MariaDB/MySQL query handler |

### DB/PostgreSQL

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `SQLConnector_PostgreSQL` | `Mantids30::DB::PostgreSQL` | `DB/PostgreSQL/sqlconnector_pgsql.h` | PostgreSQL connector |
| `Query_PostgreSQL` | `Mantids30::DB::PostgreSQL` | `DB/PostgreSQL/query_pgsql.h` | PostgreSQL query handler |

### DB/SQLite3

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `SQLConnector_SQLite3` | `Mantids30::DB::SQLite3` | `DB/SQLite3/sqlconnector_sqlite3.h` | SQLite3 connector (file or in-memory) |
| `Query_SQLite3` | `Mantids30::DB::SQLite3` | `DB/SQLite3/query_sqlite3.h` | SQLite3 query handler |

## Network

### Network/Chains

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Socket_Chain` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain.h` | Stack of layered protocols over a socket |
| `Socket_Chain_EndPointBase` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain_endpointbase.h` | Endpoint of a chain |
| `Socket_Chain_ProtocolBase` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain_protocolbase.h` | Base for chainable protocols |
| `Socket_Chain_TLS` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain_tls.h` | TLS layer |
| `Socket_Chain_AES` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain_aes.h` | AES layer |
| `Socket_Chain_XOR` | `Mantids30::Network::Chains` | `Network/Chains/socket_chain_xor.h` | XOR obfuscation layer |

### Network/Interfaces

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `NetworkInterfaceConfiguration` | `Mantids30::Network::Interfaces` | `Network/Interfaces/netifconfig.h` | Address/netmask/gateway configuration of an interface |
| `VirtualNetworkInterface` | `Mantids30::Network::Interfaces` | `Network/Interfaces/virtualnetworkinterface.h` | TAP/virtual interface management (GPLv2 on WIN32) |

### Network/Sockets

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Socket` | `Mantids30::Network::Sockets` | `Network/Sockets/socket.h` | Common socket base |
| `Socket_Stream` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_stream.h` | Stream socket base with reader/writer helpers |
| `Socket_Stream_Reader` / `Socket_Stream_Writer` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_stream_reader.h`, `socket_stream_writer.h` | Directional stream wrappers |
| `Socket_Stream_Dummy` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_stream_dummy.h` | Loopback stream for tests |
| `Socket_TCP` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_tcp.h` | TCP client socket |
| `Socket_TLS` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_tls.h` | TLS client socket |
| `Socket_UDP` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_udp.h` | UDP datagram socket |
| `Socket_UNIX` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_unix.h` | UNIX domain stream socket |
| `Socket_DatagramBase` | `Mantids30::Network::Sockets` | `Network/Sockets/socket_datagrambase.h` | Datagram base class |
| `Connector` | `Mantids30::Network::Sockets` | `Network/Sockets/connector.h` | Outbound connection helper |
| `Listener` | `Mantids30::Network::Sockets` | `Network/Sockets/listener.h` | Listening socket helper |
| `Callbacks_Socket_TCP_Client` / `_Server` | `Mantids30::Network::Sockets` | `Network/Sockets/callbacks_socket_tcp_*.h` | TCP event callbacks |
| `Callbacks_Socket_TLS` / `_Server` | `Mantids30::Network::Sockets` | `Network/Sockets/callbacks_socket_tls*.h` | TLS event callbacks |
| `StreamAcceptorThread` | `Mantids30::Network::Sockets::Acceptors` | `Network/Sockets/acceptor_thread.h` | One thread per accepted connection |
| `MultiThreaded` | `Mantids30::Network::Sockets::Acceptors` | `Network/Sockets/acceptor_multithreaded.h` | Multiple acceptor threads |
| `PoolThreaded` | `Mantids30::Network::Sockets::Acceptors` | `Network/Sockets/acceptor_poolthreaded.h` | Thread pool based acceptor |
| `StreamAcceptorThreadCallbacks`, `ThreadPoolCallbacks`, `MultiThreadCallbacks` | `Mantids30::Network::Sockets::Acceptors` | `Network/Sockets/acceptor_callbacks.h` | Acceptor hooks |
| `Bridge` | `Mantids30::Network::Sockets::NetStreams` | `Network/Sockets/streams_bridge.h` | Copies data between two streams |
| `Bridge_Thread` | `Mantids30::Network::Sockets::NetStreams` | `Network/Sockets/streams_bridge_thread.h` | Threaded stream bridge |
| `BufferedReader` | `Mantids30::Network::Sockets::NetStreams` | `Network/Sockets/streams_bufferedreader.h` | Buffered stream reader |
| `CryptoChallenge` | `Mantids30::Network::Sockets::NetStreams` | `Network/Sockets/streams_cryptochallenge.h` | Challenge/response stream decorator |

## Protocol

### Protocol/FastRPC1

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `FastRPC1` | `Mantids30::Protocol::FastRPC1` | `Protocol/FastRPC1/fastrpc.h` | FastRPC1 server |

### Protocol/HTTP

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `HTTPv1_Base` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/httpv1_base.h` | Shared request/response parsing base |
| `HTTPv1_Server` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/httpv1_server.h` | HTTP/1.1 server protocol handler |
| `HTTPv1_Client` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/httpv1_client.h` | HTTP/1.1 client protocol handler |
| `Content` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/common_content.h` | Request/response body handling |
| `ContentChunkedTransformer` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/common_content_chunked_subparser.h` | Chunked transfer decoder |
| `Date` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/common_date.h` | HTTP date parsing/formatting |
| `URLVars` / `URLVarContent` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/common_urlvars.h`, `common_urlvar_subparser.h` | URL encoded variables |
| `Version` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/common_version.h` | HTTP version value |
| `Status` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/rsp_status.h` | Response status codes |
| `CacheControl`, `Cookie`, `ETag` | `Mantids30::Protocol::HTTP::Headers` | `Protocol/HTTP/hdr_*.h` | Standard headers |
| `HSTS`, `XFrameOptions`, `XSSProtection` | `Mantids30::Protocol::HTTP::Headers::Security` | `Protocol/HTTP/hdr_sec_*.h` | Security headers |
| `RequestLine`, `Cookies_ClientSide` | `Mantids30::Protocol::HTTP::Request` | `Protocol/HTTP/req_*.h` | Request line and cookies |
| `Cookies_ServerSide` | `Mantids30::Protocol::HTTP::Response` | `Protocol/HTTP/rsp_cookies.h` | Response cookies |
| `FrameHeader`, `FrameContent` | `Mantids30::Protocol::HTTP::WebSocket` | `Protocol/HTTP/websocket_frame*.h` | WebSocket framing |
| `URL` | `Mantids30::Protocol::HTTP::Decoders` | `Protocol/HTTP/streamdecoder_url.h` | URL decoding stream stage |
| `URL` | `Mantids30::Protocol::HTTP::Encoders` | `Protocol/HTTP/streamencoder_url.h` | URL encoding stream stage |
| `APIReturn` | `Mantids30::Protocol::HTTP` | `Protocol/HTTP/api_return.h` | Standard API result envelope |

### Protocol/LineRecv

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `LineRecv` / `LineRecv_SubParser` | `Mantids30::Protocol::LineRecv` | `Protocol/LineRecv/linerecv*.h` | Line oriented protocol |

### Protocol/MIME

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `MIME_Message` | `Mantids30::Protocol::MIME` | `Protocol/MIME/mime_message.h` | Complete MIME message |
| `MIME_PartMessage` | `Mantids30::Protocol::MIME` | `Protocol/MIME/mime_partmessage.h` | MIME part |
| `MIME_Sub_Content`, `MIME_Sub_FirstBoundary`, `MIME_Sub_EndPBoundary`, `MIME_Sub_Header`, `MIME_HeaderOption` | `Mantids30::Protocol::MIME` | `Protocol/MIME/mime_sub_*.h` | MIME sub-parsers |

## Client

### Client/APISync

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `APISyncParameters` | `Mantids30::Client::APISync` | `Client/APISync/apisync.h` | APISync client connection parameters |

### Client/HTTPv1

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `HTTPv1` | `Mantids30::Client` | `Client/HTTPv1/client.h` | Blocking HTTP/1.1 client over TCP or TLS with streaming and injected connections |
| `HTTPv1::Request` | `Mantids30::Client` | `Client/HTTPv1/client.h` | URL, method, headers, upload source, response destination and protocol configuration |
| `HTTPv1::Response` | `Mantids30::Client` | `Client/HTTPv1/client.h` | Transport result, HTTP status, headers, body and protocol metadata |
| `HTTPv1::Callbacks` | `Mantids30::Client` | `Client/HTTPv1/client.h` | Headers, data, progress and completion callbacks with reception cancellation |
| `HTTPv1::Config` | `Mantids30::Client` | `Client/HTTPv1/client.h` | Timeouts, response limits and TLS configuration |
| `HTTPv1::Error` | `Mantids30::Client` | `Client/HTTPv1/client.h` | Request, connection, send, receive, size limit and cancellation errors |

See the [HTTPv1 client guide](../Mantids30/Client/HTTPv1/README.md) for usage examples,
uploads, callbacks, TLS and persistent connections.

## Server

### Server/WebRESTFul

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `APIServerCore` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_core.h` | Core HTTP server with protocol handlers |
| `APIServer_ClientHandler` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_clienthandler.h` | Per-connection HTTP handling |
| `APIServerConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiserver_config.h` | Listener/TLS configuration |
| `HTMLIEngine` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/htmliengine.h` | HTML include engine |
| `APIProxyParameters` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiproxy.h` | Reverse proxy parameters |
| `ResourcesFilter` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/resourcesfilter.h` | Resource authorization filter |
| `Engine` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/engine.h` | RESTful API server engine |
| `ClientHandler` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/clienthandler.h` | RESTful client/session handler |
| `Session` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | HTTP session with client details |
| `SessionInfo` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | Session metadata |
| `ClientDetails` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session.h` | Remote client information |
| `Session_Vars` | `Mantids30::Server::WebRESTFul::Sessions` | `Server/WebRESTFul/session_vars.h` | Session variable storage |
| `Endpoints`, `RESTfulAPIEndpointFullDefinition`, `RequestContext` | `Mantids30::Server::WebRESTFul::RESTful` | `Server/WebRESTFul/api_restful_endpoints.h` | RESTful endpoint registry |
| `OptionsHandlerConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/api_options_handler.h` | HTTP OPTIONS handling |
| `Endpoints_Options` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/endpoints_options.h` | Endpoint options |
| `Config`, `WebSocketConnection`, `Endpoint`, `Endpoints` | `Mantids30::Server::WebRESTFul::WebSocket` | `Server/WebRESTFul/api_websocket_*.h` | WebSocket endpoints and connections |
| `ReceivedAuth`, `Configuration` | `Mantids30::Server::WebRESTFul::Security` | `Server/WebRESTFul/security.h` | Authorization chain configuration |
| `APIProxyConfig` | `Mantids30::Server::WebRESTFul` | `Server/WebRESTFul/apiproxy_config.h` | Proxy configuration builder |

## Scripting

### Scripting/JSONExprEval

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `JSONEval` | `Mantids30::Scripting::JSONExprEval` | `Scripting/JSONExprEval/jsoneval.h` | JSON expression evaluator |
| `AtomicExpression`, `AtomicExpressionSide` | `Mantids30::Scripting::JSONExprEval` | `Scripting/JSONExprEval/atomicexpression*.h` | Expression operands |

### Scripting/MantidsLang

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `MantidsLang` | `Mantids30::Scripting::MantidsLang` | `Scripting/MantidsLang/mantidslang.h` | MantidsLang interpreter (streamable) |

## Runtime

### Runtime/Logs

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `AppLog`, `LogBase`, `WebLog`, `RPCLog` | `Mantids30::Runtime::Logs` | `Runtime/Logs/*.h` | Logging backends with rotation |

### Runtime/Service

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Application` | `Mantids30::Runtime::Service` | `Runtime/Service/application.h` | Application lifecycle, daemonization, signals |
| `CommandLineOption`, `GlobalArguments` | `Mantids30::Runtime::Service::Arguments` | `Runtime/Service/globalarguments.h` | Command line parsing |
| `Loader` | `Mantids30::Runtime::Service::Config` | `Runtime/Service/program_config.h` | Configuration file loader |
| `ProgramValues`, `Author` | `Mantids30::Runtime::Service::Values` | `Runtime/Service/programvalues.h` | Program metadata |

## ConfigBuilder

### ConfigBuilder/DataFormat_JWT

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Builder` | `Mantids30::ConfigBuilder::DataFormat_JWT` | `ConfigBuilder/DataFormat_JWT/config_builder.h` | JWT configuration builder |

### ConfigBuilder/Runtime_Logs

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Builder` | `Mantids30::ConfigBuilder::Runtime_Logs` | `ConfigBuilder/Runtime_Logs/config_builder.h` | Logging configuration builder |

### ConfigBuilder/Server_WebRESTFul

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `Builder` | `Mantids30::ConfigBuilder::Server_WebRESTFul` | `ConfigBuilder/Server_WebRESTFul/config_builder.h` | REST server configuration builder |

## Application

### Application/FastRPC1Client

| Class / API | Namespace | Header | Description |
|-------------|-----------|--------|-------------|
| `RPCClientApplication` | `Mantids30::Application::FastRPC1Client` | `Application/FastRPC1Client/rpcclientapplication.h` | FastRPC1 client application lifecycle |
| `RPCClientImpl` | `Mantids30::Application::FastRPC1Client` | `Application/FastRPC1Client/rpcclientimpl.h` | FastRPC1 connection and RPC calls |
| `Globals` | `Mantids30::Application::FastRPC1Client` | `Application/FastRPC1Client/globals.h` | Shared client configuration and state |

> Keep this reference in sync when public classes, APIs or headers change.
