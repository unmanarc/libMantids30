# Boost.JSON compatibility regression

This test exercises explicit conversions between standard-library and Boost
string views. Keys use substring views and embedded null bytes to verify that
lookups, setters, arrays, string lists, maps, parser input, and JWT claims retain
their exact lengths. It also checks role and scope insertion through JWT APIs.

```sh
cmake -S . -B build -DMANTIDS30_BUILD_TESTS=ON
cmake --build build --target json_compatibility_test -j 4
ctest --test-dir build -R '^json_compatibility$' --output-on-failure
```

For RHEL 8/9 with the EPEL package, add
`-DBoost_DIR=/usr/lib64/cmake/Boost-1.78.0` to the configure command.
The same source is tested against Boost 1.78 and the current local Boost version;
no conversion depends on implicit interoperability between their view types.
