# DataFormat_WebEnhancedMustache

Web Enhanced Mustache Template Engine for libMantids30.

This library extends the standard Mustache template syntax with web-specific features for server-side HTML rendering, integrating with the WebCore infrastructure.

## Features

### Standard Mustache Support

- Variables: `{{name}}`
- Sections: `{{#name}}...{{/name}}`
- Inverted sections: `{{^name}}...{{/name}}`
- Partials: `{{>partial_name}}`
- Comments: `{{!comment}}`
- Triple mustache (unescaped): `{{{name}}}`

### Enhanced Extensions

#### JavaScript Variable Declaration

```html
{{>js myVar := data.path}}
```

Generates: `<script>const myVar = {...};</script>`

#### HTTP Parameter Sources

```html
{{>get myVar := paramName}}     <!-- From GET parameters -->
{{>post myVar := paramName}}    <!-- From POST parameters -->
{{>cookie myVar := cookieName}} <!-- From cookies -->
{{>header myVar := HeaderName}} <!-- From HTTP headers -->
{{>sess myVar := sessionKey}}   <!-- From session data -->
```

#### Function Calls

```html
{{>call result := functionName({"param": "value"})}}
```

Executes an external function and optionally stores the result as a JavaScript const.

#### Template Includes

```html
{{>include path/to/template}}      <!-- Include with recursion -->
{{>include! path/to/template}}     <!-- Include without recursion -->
{{>include div:path/to/template}}  <!-- Include wrapped in <div> -->
```

#### Configuration

```html
{{>config maxIncludeDepth := 10}}
```

## API

### EnhancedMustache Class

```cpp
#include <Mantids30/DataFormat_WebEnhancedMustache/enhanced_mustache.h>

using namespace Mantids30::DataFormat::Mustache;

// Create engine
EnhancedMustache engine;

// Set context (sources of variables)
MustacheContext context;
context.addJsonData(myJsonData);
context.addAbstractVars(myVarsPtr, "session");
engine.setContext(std::move(context));

// Set HTTP sources
engine.setHTTPGetVars(getVars);
engine.setHTTPPostVars(postVars);
engine.setCookies(cookiesMap);
engine.setHeaders(headersMap);

// Set callbacks
engine.setFunctionCallback([](const std::string& name, const Json::Value& params) {
    // Handle function call
    return Json::Value();
});

engine.setIncludeCallback([](const std::string& path) {
    // Load and return template content
    return std::string();
});

// Render
std::string result = engine.render(templateString);

// Or use streams
engine.renderFromStream(inputStream);
engine.renderToStream(templateString, outputStream);
engine.renderStreamToStream(inputStream, outputStream);
```

### MustacheContext Class

Manages multiple sources of variables with priority ordering:

```cpp
MustacheContext context;

// Add JSON data
context.addJsonData(jsonValue);

// Add Abstract Vars with source name
context.addAbstractVars(varsPtr, "session");

// Add custom vars
context.addCustomVars(jsonValue, priority, "custom");

// Resolve a variable
Json::Value value = context.resolve("path.to.var");
```

## Dependencies

- jsoncpp
- Mantids30_Helpers
- Mantids30_Memory

## Building

The library is built as part of the libMantids30 project using CMake:

```bash
cmake ..
make
```

## License

See LICENSE file in the project root.