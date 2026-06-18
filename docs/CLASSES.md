
# Core Classes Reference (namespace Mantids30)

## **TODO**: This document is outdated. we must update it.


## Database Layer (namespace Mantids30::Database)
| Class | Description |
|-------|-------------|
| `SQLConnector` | Base class for database connections |
| `Query` | Base class for SQL query execution with prepared statements |
| `DatabaseCredentials` | Database credentials storage |
| `SQLConnector_SQLite3` | SQLite3 database connector (in-memory and file-backed) |
| `Query_SQLite3` | SQLite3 query handler |
| `SQLConnector_PostgreSQL` | PostgreSQL database connector |
| `Query_PostgreSQL` | PostgreSQL query handler |
| `SQLConnector_MariaDB` | MariaDB/MySQL database connector |
| `Query_MariaDB` | MariaDB/MySQL query handler |

## Memory System - Abstract Variables (namespace: Mantids30::Memory)
| Class | Description |
|-------|-------------|
| `Abstract::Var` | Base class for all abstract typed variables |
| `Abstract::BINARY` | Binary data variable type |
| `Abstract::BOOL` | Boolean variable type |
| `Abstract::DATETIME` | DateTime variable type |
| `Abstract::DOUBLE` | Double-precision floating-point variable type |
| `Abstract::INT8` | 8-bit signed integer variable type |
| `Abstract::INT16` | 16-bit signed integer variable type |
| `Abstract::INT32` | 32-bit signed integer variable type |
| `Abstract::INT64` | 64-bit signed integer variable type |
| `Abstract::IPV4` | IPv4 address variable type |
| `Abstract::IPV6` | IPv6 address variable type |
| `Abstract::MACADDR` | MAC address variable type |
| `Abstract::PTR` | Pointer variable type |
| `Abstract::STRING` | String variable type |
| `Abstract::STRINGLIST` | List of strings variable type |
| `Abstract::UINT8` | 8-bit unsigned integer variable type |
| `Abstract::UINT16` | 16-bit unsigned integer variable type |
| `Abstract::UINT32` | 32-bit unsigned integer variable type |
| `Abstract::UINT64` | 64-bit unsigned integer variable type |
| `Abstract::VARCHAR` | Variable-length character variable type |
| `Abstract::VariableMap` | Map/dictionary variable type |

## Memory System - Containers (namespace: Mantids30::Memory)

| Class | Description |
|-------|-------------|
| `Containers::B_Chunks` | Chunk-based binary container implementation |
| `Containers::B_MEM` | Linear memory binary container |
| `Containers::B_MMAP` | Memory-mapped file binary container |
| `Containers::B_Ref` | Memory referenced binary container |

| `Streams::StreamableObject` | Base class for streamable objects |

## Network - Sockets (namespace: Mantids30::Network)
| Class | Description |
|-------|-------------|
| `Sockets::Socket_TCP` | TCP stream socket |
| `Sockets::Socket_TLS` | TLS stream socket |
| `Sockets::Socket_UDP` | UDP Datagram socket |
| `Sockets::Socket_UNIX` | Unix stream socket |
| `Sockets::Socket_Chain` | Chain Stream Protocols |

## Network - Protocols (namespace: Mantids30::Network)
| Class | Description |
|-------|-------------|
| `Protocol::WebSocket` | WebSocket protocol handler |
| `Protocol::HTTP::HTTPv1_Server` | HTTP/1.1 server protocol handler |
| `Protocol::HTTP::HTTPv1_Client` | HTTP/1.1 client protocol handler |
| `Protocol::FastRPC::FastRPC1` | FastRPC1 RPC server |
| `Protocol::FastRPC::FastRPC3` | FastRPC3 RPC server with JWT Authentication |
| `Protocol::MIME::MIME_Message` | MIME multipart handler |
| `Protocol::LineRecv` | Line-based protocol parser |

## Network - Interfaces (namespace: Mantids30::Network)
| Class | Description |
|-------|-------------|
| `Interfaces::Manager` | Network physical interface detection and management |

## API Endpoints and Sessions
| Class | Description |
|-------|-------------|
| `Sessions::Session` | HTTP session with request/response handling |
| `Sessions::SessionVars` | Session variables storage |
| `API::RESTful::Endpoints` | RESTful API endpoint handlers |
| `API::Monolith::Endpoints` | Monolith API endpoint handlers |
| `API::WebSocket::Endpoint` | WebSocket endpoint configuration |
| `API::WebSocket::Connection` | WebSocket connection handler |
| `API::WebSocket::Endpoints` | Collection of WebSocket endpoints |
| `API::EndpointsRequirementsMap` | Endpoint requirements mapping |
| `API::OptionsHandler` | HTTP OPTIONS request handler |

## Web Servers
| Class | Description |
|-------|-------------|
| `Server::RESTfulWebAPI::ServerImpl` | RESTful Web API server implementation base |
| `Server::MonolithWebAPI::ServerImpl` | Monolith Web API server implementation base |
| `Server::WebCore::...` | Core web server components |

## Threading
| Class | Description |
|-------|-------------|
| `Threads::Thread` | Thread management class |
| `Threads::Semaphore` | Semaphore for thread synchronization |
| `Threads::RW::Mutex` | Read/Write mutex lock |
| `Threads::Pool` | Thread pool with semaphore-based control |

## Thread-Safe Containers
| Class | Description |
|-------|-------------|
| `Helpers::StdQueue<T>` | Thread-safe queue container |
| `Helpers::StdList<T>` | Thread-safe list container |
| `Helpers::StdMap<K,V>` | Thread-safe map container |
| `Helpers::StdSet<T>` | Thread-safe set container |
| `Helpers::StdVector<T>` | Thread-safe vector container |

## Helpers & Utilities
| Class/Function | Description |
|----------------|-------------|
| `Helpers::Crypto` | Cryptographic utilities (SHA256, HMAC, hashing) |
| `Helpers::Random` | Random string and number generation |
| `Helpers::File` | File operations utilities |
| `Helpers::JSON` | JSON parsing and serialization helpers |
| `Helpers::TOTP` | Time-based One-Time Password generation |
| `Helpers::Datatables` | DataTables helper for server-side processing |
| `Helpers::Encoders` | Data encoding utilities (Base64, URL, etc.) |
| `Helpers::StrConv` | String conversion utilities |
| `Helpers::OS` | OS-specific utility functions |
| `Helpers::AppExec` | Application execution utilities |
| `Helpers::SafeInt` | Safe integer conversion utilities |
| `Helpers::Callbacks` | Callback mechanism utilities |

## Data Formats
| Class | Description |
|-------|-------------|
| `DataFormat::JWT::JWT` | JWT token creation, validation, and revocation |

## File Formats
| Class | Description |
|-------|-------------|
| `FileFormats::VarsFile` | VarsFile configuration format handler |

## Logging
| Class | Description |
|-------|-------------|
| `Program::Logs::AppLog` | Application logging with structured log levels |
| `Program::Logs::LogLevel` | Log level enumeration (DEBUG, INFO, WARN, ERR, CRITICAL) |

## Application Lifecycle
| Class | Description |
|-------|-------------|
| `Program::Service::Application` | Application lifecycle and service management |

## Scripting
| Class | Description |
|-------|-------------|
| `Scripts::JSONExprEval::Evaluator` | JSON Expression Evaluation engine |

## FastRPC Client
| Class | Description |
|-------|-------------|
| `App::FastRPC1Client::RPCClientImpl` | FastRPC1 client implementation |
| `App::FastRPC1Client::RPCClientApplication` | FastRPC1 client application framework |
