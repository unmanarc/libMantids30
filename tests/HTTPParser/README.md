# Raw HTTP parser tests

These tests feed raw `std::string` messages directly into `HTTPv1_Client` and
`HTTPv1_Server`. They use memory streams for outgoing data and require no sockets,
DNS, TLS, external servers or test framework.

## Run

From the repository root:

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON
cmake --build build --target http_parser_test -j 4
ctest --test-dir build -R '^http_parser_' --output-on-failure
```

Run one suite or filter by a case-name substring:

```sh
build/tests/HTTPParser/http_parser_test client_valid
build/tests/HTTPParser/http_parser_test server_invalid conflicting_content_lengths
build/tests/HTTPParser/http_parser_test streaming every_truncated_prefix
```

Exit codes: `0` means all selected cases passed, `1` means assertions failed,
and `2` means invalid arguments, an unknown suite or no matching cases. All
selected cases run even after earlier cases fail. Failures identify the case,
fragment schedule and failed assertion.

## Delivery and assertions

Each ordinary case runs with the entire message, one byte at a time, two bytes
at a time, seven bytes at a time, and a repeating `1,13,2,3,29,5` byte schedule.
The three `every_split` cases instead run once, covering every possible two-part
split of their representative message. Prefix sweeps examine every proper prefix,
including the empty input, and aggregate failures without stopping at the first.

Valid messages must consume exactly their own bytes, complete at the appropriate
time, preserve binary body bytes and expose the expected parsed fields. Fixed
length and chunked messages must complete before connection EOF; close-delimited
responses must wait for EOF. Suffix tests append another message or representation
bytes and assert that the parser leaves them untouched.

Invalid responses must produce `ERR_PARSING`. Invalid requests must fail parsing
or generate an HTTP error without invoking the application content callback.
Missing, empty or duplicate HTTP/1.1 Host and invalid Host ports explicitly expect
`400`; unsupported request major versions expect `505`; oversized request bodies
expect `413`. Destination failures must preserve `ERR_STREAM_WRITE`.

The tests enforce the client's existing policy of rejecting duplicate
Content-Length fields, even when their values match, and unsupported transfer
codings. Response Transfer-Encoding takes precedence over Content-Length;
requests containing both must be rejected before application dispatch.

## Case sets

| Suite | Cases | Expected behavior covered |
| --- | ---: | --- |
| `client_valid` | 31 | HTTP/1.0 and 1.1; fixed, empty, binary and close-delimited bodies; 404 and extension statuses; empty reason phrase; HEAD, 101, 204 and 304; multiple interim responses without metadata leakage; case-insensitive headers; optional whitespace; cookies, ETag, realm and repeated headers; chunks, hexadecimal sizes, extensions and trailers; message boundaries; header cancellation and custom destinations. |
| `client_invalid` | 39 | Invalid status lines and versions; empty, signed, suffixed, hexadecimal, list and overflowing lengths; duplicate/conflicting framing; unsupported/repeated transfer codings; invalid and truncated chunks, missing final chunk and trailers; incomplete headers and interim responses; malformed header names and NUL bytes. |
| `client_limits` | 15 | Bodies one byte below, at and above a five-byte limit for all three framing modes; per-chunk and cumulative limits; header line and count limits; data and EOF destination failures. |
| `server_valid` | 25 | HTTP/1.0 without Host and HTTP/1.1 with Host; GET, HEAD, OPTIONS `*`, PUT, PATCH and DELETE; host ports and IPv6; header case and whitespace; decoded query and form fields, repeated and empty values; cookies, Basic and Bearer authentication; fixed, empty, binary, JSON, URL form, chunked and multipart bodies; upload filename; boundaries and callback cancellation. |
| `server_invalid` | 30 | Missing/empty/duplicate Host and invalid ports; unsupported or malformed request versions and lines; truncated headers and bodies; ambiguous lengths and transfer codings; malformed chunks; invalid numeric lengths; malformed header names and NUL bytes. |
| `server_limits` | 8 | Bodies below, at and above the configured limit; request-line, header-line and header-count limits; data and EOF destination failures without application dispatch. |
| `streaming` | 10 | Every split of fixed client/server messages and chunked responses; every truncated prefix of representative fixed and chunked messages; waiting for remaining body bytes; close-delimited EOF; two pipelined requests with cookie state cleared. |

## Current results

After fixing the parser defects below, all **158 named cases / 778 case-schedule
runs** pass, including every split and truncated-prefix sweep. All **17 CTest
suites/tests** pass, including the previous TCP and TLS integration tests.

Corrections cover strict line termination and numeric parsing, header syntax and
limits, chunk extensions and request framing, IPv6 host/port splitting, empty URL
values, and avoiding duplicate EOF delivery to completed multipart containers.
The shared fixed-size subparser now reports an error on premature EOF.

## Initial results (before fixes)

The initial run against the current parsers contains **158 named cases** and
**778 case/schedule runs**, plus the internal split and prefix sweeps:

| Suite | Passed runs | Failed runs |
| --- | ---: | ---: |
| `client_valid` | 135 | 20 |
| `client_invalid` | 125 | 70 |
| `client_limits` | 70 | 5 |
| `server_valid` | 90 | 35 |
| `server_invalid` | 45 | 105 |
| `server_limits` | 35 | 5 |
| `streaming` | 23 | 15 |
| **Total** | **523** | **255** |

There are **51 distinct failing cases**, repeated across delivery schedules. All
seven CTest suites initially failed; known defects are neither skipped nor marked
as expected failures. The ten pre-existing CTest tests still pass.

Observed failures include:

- Headers without a space after the colon or with a tab are missed. The client
  also rejects valid Content-Length surrounding whitespace.
- Invalid response status tokens and protocol versions are accepted.
- Some incomplete messages return success or remain pending after EOF. Truncated
  chunk data and chunk CRLF are not consistently rejected.
- Chunk extensions fail in the client; chunked requests are dispatched as empty
  requests by the server.
- The server accepts duplicate Host, invalid ports, conflicting lengths, invalid
  numeric lengths and unsupported or ambiguous transfer codings.
- Both parsers accept malformed headers and silently discard headers exceeding
  the configured count instead of rejecting the message.
- Server IPv6 host parsing, empty query/form values and the binary multipart
  request fail their expected-field or completion assertions.

These historical failures document the baseline. The original assertions remain
in place and now pass.
