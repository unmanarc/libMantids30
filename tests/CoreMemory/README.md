# Core Memory tests

The suite contains 77 named cases in six CTest groups. Assertions throw with the
source location and expression, remain active with `NDEBUG`, and give the test
process a nonzero exit status on failure. No external test framework is required.

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target core_memory_test -j 4
ctest --test-dir build -L memory --output-on-failure
```

Run a group directly for individual case results:

```sh
./build/tests/CoreMemory/core_memory_test containers
```

| Group | Cases | Contracts checked |
| --- | ---: | --- |
| helpers | 2 | All 65,536 byte pairs in sensitive/ASCII-insensitive comparisons; embedded NULs; copy/move guards; overlapping moves in both directions around 64 KiB boundaries. |
| containers | 17 | Empty/reused buffers, binary data, sizes around 8/64 KiB, all offsets in small buffers, output guard bytes, chunk order/offsets, partial writes at chunk limits, zero chunk size, truncation/displacement boundaries, capacity/overflow rejection, bounded references, source shrink, case-sensitive searches, delimiter copy/removal, destination failures, assignment and stream failures. |
| files | 11 | Empty and binary mappings, prepend/append/displace/truncate/reopen, read-only protection, invalid ranges/paths, persistence/deletion policy, ownership transfer, memory-to-disk threshold, spill failure, cleanup, and binary file streams. |
| streams | 10 | Partial writes, stalled/oversized/failed writes, status propagation, explicit EOF, binary strings, null sink accounting, transformations, and formatted writes including embedded NULs. |
| parsers | 14 | Buffered/direct delimiters, all two-fragment split points and bytewise overlaps, binary delimiters, unconsumed tails, fixed-size/direct/connection-end modes, EOF and partial prefixes, invalid configurations, buffer/recursion limits, protocol transitions, initialization/parse errors and pre-stream input. |
| variables | 23 | Every signed/unsigned integer width at min/max and overflow, strict numeric parsing, non-terminated string views, copies/types/null state, JSON round trips, boolean literals, strings, bounded VARCHAR, binary/Base64, CSV quoting/empty fields, IPv4/IPv6/MAC, UTC dates, pointer parsing, factories, maps and multi-value conversion. |

Container mutation tests run 6,000 operations against `std::string`, with seeds
`0`, `1`, `42` and `0xC0FFEE`. Search tests compare 250 generated buffers against
`std::string::find`, with seed `20261116`. Temporary files live in independently
owned directories so CTest groups can run concurrently.

## Sanitizers

Use a build directory on the same filesystem as the source: the project's RPM
configuration currently renames a generated file into the source tree.

```sh
cmake -S . -B build/memory-sanitizers \
  -DMANTIDS30_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined' \
  '-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build build/memory-sanitizers --target core_memory_test -j 4
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/memory-sanitizers -L memory --output-on-failure
```

LeakSanitizer requires an environment without a ptrace-based tracer. If another
library is preloaded before ASan, load the compiler's ASan runtime first.

## Scope and existing conventions

- `VALIDATOR` is currently unimplemented; its test checks explicit failure.
- Zero-length `append` returns `nullopt`, while zero-length `prepend` returns zero.
- Integer `fromString` resets the value on failure; `DOUBLE` preserves its value.
- Boolean parsing accepts the existing true literals and maps other inputs to false.
- `FileMap` assignment transfers ownership from the source.
- Tests use bounded real allocations. They do not claim exhaustive coverage of
  multi-gigabyte mappings, allocation failures, descriptor exhaustion, concurrent
  mutation or external-process streams. Platform-specific behavior must also be
  validated on the corresponding platform.
