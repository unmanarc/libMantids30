# WebRESTFul tests

Build and run with the project's optional test configuration:

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON
cmake --build build --target webrestful_test -j 4
ctest --test-dir build -R '^webrestful_' --output-on-failure
```

Run one suite or filter its cases:

```sh
build/tests/WebRESTFul/webrestful_test integration
build/tests/WebRESTFul/webrestful_test integration keep_alive
```

The test executable reports each failing case and returns a failure when no cases
match. CTest registers eight independently runnable suites:

| Suite | Coverage |
| --- | --- |
| `routing` | Five HTTP verbs, enum/string dispatch, exact paths, invalid methods, replacement, full definitions, null callbacks/output, application response propagation |
| `security` | All combinations of header/cookie requirements, received credentials, scope subsets and admin status; case-sensitive scopes and configuration reset |
| `input` | JSON objects, arrays, scalars and null; encoded GET JSON, invalid queries, empty input reset, client metadata and JWT forwarding |
| `cors` | Defaults, fluent setters, replacement, deduplication, exact origins, wildcard, optional headers, max-age boundaries and endpoint overrides |
| `filters` | Regex matching, required/rejected scopes and roles, session rules, admin bypass limits, action ordering and termination, empty actions, configuration loading, aliases, atomic failure and document-root autoload |
| `sessions` | Identity/token snapshots, activity expiration without sleeps, variable CRUD and copy isolation, concurrent access and client metadata serialization |
| `integration` | Versioned HTTP routing, JSON request/response serialization, three fragmentation sizes, signed JWT validation, app/type/expiry/signature failures, scope checks, credential channels and precedence, keep-alive authentication isolation, custom cookies, origin checks, dynamic validators, preflight, response cookies/headers and early rejection |
| `websocket` | Default endpoint security, event callback dispatch, JSON translation toggle, invalid/missing events, URI lookup and empty connection registries |

Integration cases use the real Engine factory, ClientHandler, HTTP request parser,
and HTTP response parser over an in-memory Socket_Stream_Dummy. No listener ports,
external services, or uFastAuthD3 installation are required. The consumer patterns
reviewed were uFastAuthD3's LoginPortal, AdminPortal, AppSync and SessionAuthHandler:
versioned registration, cookie/header JWT access, scoped endpoints, JSON bodies,
and per-application token/origin validation.

WebSocket cases cover the endpoint layer; they do not establish network WebSocket
sessions or exercise frame transport and live subscription delivery. Session tests
exercise stored state rather than the persistent session login engine.

The regression cases cover two fixes found while adding this suite: matching
resource filters with no actions must not access an empty vector, and keep-alive
requests must reset JWT authentication and session metadata before validation.
