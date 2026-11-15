# Mantids30 DataFormat JSON

Mantids30 DataFormat JSON is a C++ library that provides helper functions for parsing, accessing and serializing JSON data on top of Boost.JSON. It also provides a `StreamableJSON` streamable object for streaming JSON payloads.

## Features
- JSON parsing and serialization helpers (`parse`, `toString`, `Reader`)
- Safe object member accessors with default values (`ASSTRING`, `ASINT`, `ASBOOL`, ...)
- Direct value accessors for an already-resolved value (`ASSTRING_D`, `ASINT_D`, ...)
- Array index accessors (`ARRAY_ASSTRING`, `ARRAY_ASINT`, ...)
- jsoncpp-style JSON path resolution (`Path`)
- Conversion helpers between JSON and C++ containers (`toMap`, `toStringList`, `fromSet`, ...)
- `StreamableJSON` streamable object for writing JSON values to streams

## Dependencies
- Boost C++ Libraries (boost.json)

## Usage

Include the JSON header in your project and use the provided namespaces `Mantids30::Helpers::JSON` and `Mantids30::Helpers`.

```cpp
#include "json.h"

using namespace Mantids30::Helpers::JSON;
```

### Parsing a JSON document

To parse a JSON document, use the `parse` function or the `Reader` class.

```cpp
std::string document = R"({"name": "example", "value": 42, "tags": ["a", "b"]})";
boost::json::value root;
bool ok = parse(document, root);

// Or using the Reader class:
Reader reader;
ok = reader.parse(document, root);
std::string errors = reader.getFormattedErrorMessages();
```

### Accessing values

Use the safe accessors to read object members with a fallback default value.

```cpp
std::string name = ASSTRING(root, "name", "unknown");
int value = ASINT(root, "value", 0);
std::string firstTag = ARRAY_ASSTRING(root, "tags", 0, "");
```

Direct accessors operate on a value that is already the target element (no key lookup):

```cpp
const boost::json::value *tags = root.if_contains("tags");
std::string firstTag = ASSTRING_D(*tags, "");
```

### Resolving a JSON path

The `Path` class resolves jsoncpp-style path expressions such as `.name`, `["name"]` or `[0]`.

```cpp
Path path("$.tags[0]");
const boost::json::value *tag = path.resolve(root);
```

### Converting to C++ containers

```cpp
std::list<std::string> tags = toStringList(root, "tags");
std::map<std::string, std::string> flat = toMap(root);
```

### Using StreamableJSON

`StreamableJSON` implements `Mantids30::Memory::Streams::StreamableObject` so JSON payloads can be streamed like any other streamable object.

```cpp
#include "streamable_json.h"

Mantids30::Memory::Streams::StreamableJSON json;
json.setValue(R"({"message": "hello"})");
boost::json::value *value = json.processValue();
```
