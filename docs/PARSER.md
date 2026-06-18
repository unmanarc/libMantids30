# Parsing System

The parsing system is the core engine that enables all protocol implementations in libMantids30 (HTTP, MIME, WebSocket, FastRPC, LineRecv, etc.). It follows a layered, composable architecture that allows building complex protocol stacks by chaining independent parsing stages.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                     Protocol Layer                      │
│  (HTTPv1_Base, FastRPC, MIME, LineRecv, WebSocket...)   │
│                   Inherits from Parser                  │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│                      Parser                             │
│  • Orchestrates protocol lifecycle (init/parse/end)     │
│  • Chains multiple SubParsers sequentially              │
│  • TTL-based infinite loop protection                   │
│  • Client/Server mode abstraction                       │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│                     SubParser                           │
│  • Atomic parsing engine with multiple strategies       │
│  • Buffer management (B_Chunks + B_Ref)                 │
│  • Multi-delimiter matching                             │
│  • Streams parsed data to upstream StreamableObject     │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│                  StreamableObject                       │
│  • Abstract I/O interface                               │
│  • Unified write()/streamTo() contract                  │
│  • Foundation for all data containers                   │
└─────────────────────────────────────────────────────────┘
```

## Core Components

| Component | File | Role |
|-----------|------|------|
| **StreamableObject** | `Memory/streamable_object.h` | Base abstract I/O interface. Defines `write()`, `streamTo()`, and `writeStatus` for unified data streaming |
| **SubParser** | `Memory/subparser.h` | Atomic parsing engine. Handles buffer management, strategy selection, and data extraction |
| **Parser** | `Memory/parser.h` | High-level protocol orchestrator. Manages SubParser chaining, protocol lifecycle, and TTL protection |

## Parsing Strategies

The `SubParser` class supports multiple parsing strategies via `ParseStrategy` enum, allowing the same engine to handle vastly different protocol formats:

| Strategy | Description | Use Cases |
|----------|-------------|-----------|
| **DELIMITER** | Parse until a specific delimiter is found | HTTP headers (`\r\n`), line-based protocols |
| **SIZE** | Parse until a pre-defined byte count is reached | Fixed-size binary headers, TLS records |
| **VALIDATOR** | Parse until a custom validation function succeeds | JSON/XML content validation |
| **CONNECTION_END** | Accumulate data until the connection closes | Simple streaming protocols |
| **DIRECT** | Parse immediately without waiting | Throughput-optimized pipelines |
| **DIRECT_DELIMITER** | Parse directly until a multi-delimiter boundary | MIME multipart boundaries |
| **MULTIDELIMITER** | Parse until ANY of several delimiters is matched | Protocol negotiation, response detection |

## Key Advantages

| Advantage | Description |
|-----------|-------------|
| **Protocol Chaining** | Parsers can be chained sequentially (`changeToNextParser()`), enabling layered protocols: TLS over TLS, HTTP with MIME bodies, WebSocket frames over HTTP upgrades, etc. |
| **Strategy Flexibility** | A single SubParser instance can switch strategies mid-stream, adapting to different phases of the same protocol |
| **Zero Code Duplication** | All protocols (HTTP, MIME, FastRPC, LineRecv) share the same SubParser engine — no duplicated parsing logic |
| **Efficient Buffering** | Uses `B_Chunks` (chunk-based allocation) and `B_Ref` (referenced buffers) for minimal memory copies during parsing |
| **TTL Protection** | Built-in Time-To-Live counter (`setMaxTTL()`) prevents infinite parsing loops on malformed data |
| **Bidirectional Mode** | The same parser works in both client and server modes (`clientMode` flag), reducing code duplication |
| **Multi-Delimiter Matching** | Native support for matching against multiple delimiter patterns simultaneously, with the matched delimiter available via `getFoundDelimiter()` |
| **Streamable Interface** | The unified `StreamableObject` interface allows any data container to act as both a source and destination in the parsing pipeline |

## Implementing a Custom Protocol

To implement a new protocol, inherit from `Parser` and override three virtual methods:

```cpp
class MyProtocol : public Mantids30::Memory::Streams::Parser
{
public:
    MyProtocol(bool clientMode, std::shared_ptr<Memory::Streams::StreamableObject> socket)
        : Parser(socket, clientMode) {}

protected:
    // Initialize protocol state and first SubParser
    bool initProtocol() override
    {
        initSubParser(&m_headerParser);
        return true;
    }

    // Clean up when protocol finishes
    void endProtocol() override
    {
        // Cleanup resources
    }

    // Chain to the next SubParser (e.g., headers -> body)
    bool changeToNextParser() override
    {
        if (m_currentSubParser == &m_headerParser)
        {
            initSubParser(&m_bodyParser);
            m_bodyParser.setParseDataTargetSize(parsedContentLength);
            return true;
        }
        return false; // Protocol complete
    }

private:
    Memory::Streams::SubParser m_headerParser;
    Memory::Streams::SubParser m_bodyParser;
};
```

## How Protocols Use the Parser

Each protocol implementation follows the same pattern:

1. **HTTP** (`Protocol_HTTP/`): Uses chained SubParsers for request line → headers → body (with chunked encoding support via `common_content_chunked_subparser`)
2. **MIME** (`Protocol_MIME/`): Uses `MULTIDELIMITER` strategy to parse multipart boundaries
3. **LineRecv** (`Protocol_LineRecv/`): Uses `DELIMITER` strategy with configurable line terminators
4. **FastRPC** (`Protocol_FastRPC1/`, `Protocol_FastRPC3/`): Uses `SIZE` strategy for length-prefixed message frames
5. **WebSocket** (`Protocol_HTTP/websocket_*.h`): Uses `SIZE` strategy for frame payload lengths after HTTP upgrade