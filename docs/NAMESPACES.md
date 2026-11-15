# C++ namespace layout

Public namespaces begin with `Mantids30::<Group>::<Module>`, matching the
installed header directory. Nested namespaces describe APIs within a module,
such as `Core::Memory::Streams` or `Protocols::HTTP::Headers`.

| Header directory | Namespace |
| --- | --- |
| `Core/Helpers` | `Mantids30::Core::Helpers` |
| `Core/Memory` | `Mantids30::Core::Memory::{Abstract,Containers,Streams}` |
| `Core/Threads` | `Mantids30::Core::Threads`, with `Safe` and `Pool` |
| `DataFormats/JSON` | `Mantids30::DataFormats::JSON` |
| `DataFormats/JWT` | `Mantids30::DataFormats::JWT` |
| `DB/Common` | `Mantids30::DB::Common` |
| `DB/MariaDB`, `DB/PostgreSQL`, `DB/SQLite3` | `Mantids30::DB::{MariaDB,PostgreSQL,SQLite3}` |
| `Network/Interfaces`, `Network/Sockets`, `Network/Chains` | `Mantids30::Network::{Interfaces,Sockets,Chains}` |
| `Protocols/HTTP` | `Mantids30::Protocols::HTTP`, including framing under `WebSocket` and URL codecs under `Encoders` / `Decoders` |
| `Protocols/MIME`, `Protocols/LineRecv`, `Protocols/FastRPC1`, `Protocols/APISync` | `Mantids30::Protocols::{MIME,LineRecv,FastRPC1,APISync}` |
| `Runtime/Logs`, `Runtime/Service` | `Mantids30::Runtime::{Logs,Service}` |
| `Scripting/JSONExprEval`, `Scripting/MantidsLang` | `Mantids30::Scripting::{JSONExprEval,MantidsLang}` |
| `Applications/FastRPC1Client` | `Mantids30::Applications::FastRPC1Client` |
| `Servers/RESTfulWebServer` | `Mantids30::Servers::RESTfulWebServer`, with `RESTful`, `WebSocket`, `Sessions` and `Security` |
| `ConfigBuilder/JWT`, `ConfigBuilder/Logs`, `ConfigBuilder/REST` | `Mantids30::ConfigBuilder::{JWT,Logs,REST}` |

## Usage

Configuration factories use the class name `Builder` within each
`ConfigBuilder` module. Their library and pkg-config names are
`Mantids30_ConfigBuilder_JWT`, `Mantids30_ConfigBuilder_Logs` and
`Mantids30_ConfigBuilder_REST`.

Shared database interfaces live in `DB/Common` and are provided by
`Mantids30_DB`. Each backend has its own namespace and library.

Service startup (`StartApplication`, `globalArgs`) belongs to
`Runtime::Service`; the configuration loader belongs to
`Runtime::Service::Config`. Proxy configuration belongs to the server module.

JSON helpers, `DataTables` and `StreamableJSON` belong to `DataFormats::JSON`.
`APIReturn` belongs to `Protocols::HTTP`. WebSocket frames belong to
`Protocols::HTTP::WebSocket`, while server endpoints and connections belong
to `Servers::RESTfulWebServer::WebSocket`.

Use local namespace aliases to keep application code concise:

```cpp
#include <Mantids30/ConfigBuilder/Logs/config_builder.h>
#include <Mantids30/ConfigBuilder/REST/config_builder.h>
#include <Mantids30/DB/SQLite3/sqlconnector_sqlite3.h>

namespace Logging = Mantids30::ConfigBuilder::Logs;
namespace REST = Mantids30::ConfigBuilder::REST;
namespace SQLite = Mantids30::DB::SQLite3;

auto log = Logging::Builder::createInitLog();
SQLite::SQLConnector_SQLite3 database;
// REST::Builder::createRESTfulEngine(...) creates the configured server.
```
