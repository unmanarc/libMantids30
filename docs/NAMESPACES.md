# C++ namespace layout

Public namespaces begin with `Mantids30::<Group>`. Modules containing a single
class with the same name expose that class directly in the group namespace:
`DataFormat::JWT`, `Protocol::FastRPC1` and `Scripting::MantidsLang`.
Other modules use `Mantids30::<Group>::<Module>`, matching the installed header
directory. Nested namespaces describe APIs within a module, such as
`Core::Memory::Streams` or `Protocol::HTTP::Headers`.

| Header directory | Namespace |
| --- | --- |
| `Core/Helpers` | `Mantids30::Core::Helpers` |
| `Core/Memory` | `Mantids30::Core::Memory::{Abstract,Containers,Streams}` |
| `Core/Threads` | `Mantids30::Core::Threads`, with `Safe` and `Pool` |
| `DataFormat/JSON` | `Mantids30::DataFormat::JSON` |
| `DataFormat/JWT` | `Mantids30::DataFormat` |
| `DB/Common` | `Mantids30::DB::Common` |
| `DB/MariaDB`, `DB/PostgreSQL`, `DB/SQLite3` | `Mantids30::DB::{MariaDB,PostgreSQL,SQLite3}` |
| `Network/Interfaces`, `Network/Sockets`, `Network/Chains` | `Mantids30::Network::{Interfaces,Sockets,Chains}` |
| `Protocol/HTTP` | `Mantids30::Protocol::HTTP`, including framing under `WebSocket` and URL codecs under `Encoders` / `Decoders` |
| `Client/APISync` | `Mantids30::Client::APISync` |
| `Protocol/MIME`, `Protocol/LineRecv` | `Mantids30::Protocol::{MIME,LineRecv}` |
| `Protocol/FastRPC1` | `Mantids30::Protocol` |
| `Runtime/Logs`, `Runtime/Service` | `Mantids30::Runtime::{Logs,Service}` |
| `Scripting/JSONExprEval` | `Mantids30::Scripting::JSONExprEval` |
| `Scripting/MantidsLang` | `Mantids30::Scripting` |
| `Application/FastRPC1Client` | `Mantids30::Application::FastRPC1Client` |
| `Server/WebRESTFul` | `Mantids30::Server::WebRESTFul`, with `RESTful`, `WebSocket`, `Sessions` and `Security` |
| `ConfigBuilder/DataFormat_JWT`, `ConfigBuilder/Runtime_Logs`, `ConfigBuilder/Server_WebRESTFul` | `Mantids30::ConfigBuilder::{DataFormat_JWT,Runtime_Logs,Server_WebRESTFul}` |

## Usage

Library targets and pkg-config packages use `Mantids30_<Group>_<Module>`,
matching the header directory with `/` replaced by `_`. For example,
`Core/Memory` provides `Mantids30_Core_Memory`, and `Scripting/MantidsLang`
provides `Mantids30_Scripting_MantidsLang`.

Configuration factories use the class name `Builder` within each
`ConfigBuilder` module. Their library and pkg-config names are
`Mantids30_ConfigBuilder_DataFormat_JWT`, `Mantids30_ConfigBuilder_Runtime_Logs` and
`Mantids30_ConfigBuilder_Server_WebRESTFul`.

Shared database interfaces live in `DB/Common` and are provided by
`Mantids30_DB_Common`. Each backend has its own namespace and library.

Service startup (`StartApplication`, `globalArgs`) belongs to
`Runtime::Service`; the configuration loader belongs to
`Runtime::Service::Config`. Proxy configuration belongs to the server module.

JSON helpers, `DataTables` and `StreamableJSON` belong to `DataFormat::JSON`.
`APIReturn` belongs to `Protocol::HTTP`. WebSocket frames belong to
`Protocol::HTTP::WebSocket`, while server endpoints and connections belong
to `Server::WebRESTFul::WebSocket`.

Use local namespace aliases to keep application code concise:

```cpp
#include <Mantids30/ConfigBuilder/Runtime_Logs/config_builder.h>
#include <Mantids30/ConfigBuilder/Server_WebRESTFul/config_builder.h>
#include <Mantids30/DB/SQLite3/sqlconnector_sqlite3.h>

namespace Logging = Mantids30::ConfigBuilder::Runtime_Logs;
namespace REST = Mantids30::ConfigBuilder::Server_WebRESTFul;
namespace SQLite = Mantids30::DB::SQLite3;

auto log = Logging::Builder::createInitLog();
SQLite::SQLConnector_SQLite3 database;
// REST::Builder::createRESTfulEngine(...) creates the configured server.
```
