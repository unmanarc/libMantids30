# Socket regression tests

Build and run on Linux with C++17, Boost.JSON, OpenSSL, Python 3, and permission
to create loopback TCP/UDP and UNIX sockets:

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON
cmake --build build --target sockets_test -j 4
ctest --test-dir build -R '^sockets_' --output-on-failure -j 4
```

The suite registers 37 independent cases when Python and the OpenSSL executable
are available (27 otherwise). Every case has a CTest timeout. Network ports are
allocated by the OS, UNIX paths include the process ID, and each TLS fixture uses
a temporary certificate. No external server or DNS service is required.

Coverage:

- Descriptor ownership, duplicate binds, failed operations, repeated close,
  reconnect/relisten, and descriptor leaks.
- TCP IPv4/IPv6 endpoint metadata, explicit source binding, half close, EOF,
  DNS fallback across address families, keepalive parameters, TCP_NODELAY,
  timeout inheritance, and descriptors above FD_SETSIZE.
- Fragmented binary transfers, partial progress on timeout, write backpressure,
  read/write errors, signal interruption, zero-length operations, integer byte
  order, length-prefixed strings/blocks, allocation overflow, and delimiters
  crossing 64 KiB boundaries.
- Buffered line reading, retained data after insufficient destination capacity,
  and delimiter-free full buffers.
- UDP datagram boundaries, empty and large datagrams, source addresses, read
  timeout/error reporting, and block copy/move/free ownership.
- UNIX transfers, invalid paths, failed-connect cleanup, backlog timeout, and
  protection of regular files at the requested socket path.
- Concurrent clients through pooled and multithreaded acceptors, shutdown waking
  blocked accept/read calls, and immediate stop racing with pool startup.
- TLS client/server data exchange, trust/hostname failures, missing trust
  configuration, explicit verification bypass, clean versus truncated EOF,
  silent-peer shutdown/destruction, and client/server handshake timeouts.
- Connection diagnostics retain operation, destination, numeric error code, and
  the original operation-specific Winsock explanations.
- Debug output ownership, including preserving stderr.

IPv6 cases return 77 (CTest skip) only when the environment does not support or
permit IPv6 sockets/loopback. The high-descriptor test also skips if the process
file-descriptor limit is too low. Windows and other operating systems are not
validated by this suite. Acceptor concurrency tests cover startup, client
processing, IP-limit callbacks, and shutdown; they do not establish freedom from every possible
thread interleaving.

For ASan/UBSan, instrument the libraries as well as the test executable:

```sh
cmake -S . -B build/sockets-sanitizers \
  -DMANTIDS30_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build/sockets-sanitizers --target sockets_test -j 4
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/sockets-sanitizers -R '^sockets_' --output-on-failure -j 4
```

Hosts that preload monitoring libraries may need
`ASAN_OPTIONS=verify_asan_link_order=0:detect_leaks=1` to run ASan.

Behavior changes covered by these regressions:

- `Block::socketAddress` now uses sockaddr_storage to retain IPv6 addresses;
  datagram blocks own independent storage when copied, and free() is idempotent.
- BufferedReader's pointer overload takes destination capacity in `*len` and
  returns the required/copied size there. Insufficient capacity returns
  READ_FULL and leaves the buffered line available for retry.
- A TLS client configured to VALIDATE requires a CA or system trust. Missing
  trust no longer silently changes the client to NOVALIDATE. Explicit
  NOVALIDATE and PSK configurations remain supported.
- TLS shutdown sends close_notify without waiting indefinitely for the peer;
  it then shuts down the requested transport direction. Under backpressure the
  close alert is best effort, so a peer can observe a truncated TLS close.

TLS shutdown preserves the distinction between incomplete confirmation (`-2`)
and failure (`-3`), records details in the TLS error list, and still shuts down
the transport without waiting indefinitely for a peer.

The compatibility audit also checks that descriptor adoption preserves existing
OS read/write timeouts, TCP clients retain their original unlimited read default
unless overridden, readFull() rejects a null destination even for an empty read,
and pooled acceptors release their registered listeners after stopping. The
original explicit port lookup is retained, with an additional IPv6 branch.
OpenSSL errors queued before an operation are collected in the TLS error list
instead of silently discarded. TLS shutdown retains a second nonblocking step
for peer confirmation and the uninitialized-handle status (`-4`).
