# Contributing to libMantids30

Thanks for your interest in improving libMantids30. This document describes how to build
the project, the coding conventions and the pull request process.

## Before you start

- Read [README.md](README.md) and [docs/CLASSES.md](docs/CLASSES.md) to understand the
  module layout.
- For security issues do **not** open a public issue; follow [SECURITY.md](SECURITY.md).
- By participating you agree to the [Code of Conduct](CODE_OF_CONDUCT.md).

## Development environment

The supported toolchain is C++17 with CMake >= 3.10. All maintained distributions must keep
working: RHEL/Alma/Rocky 7-10, Ubuntu 20.04-26.04, Debian 12/13, Fedora and MSYS2/MinGW64.

Minimal local build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON
cmake --build build --parallel "$(nproc)"
```

Before opening a pull request, also verify the static build:

```bash
cmake -S . -B build-static -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build-static --parallel "$(nproc)"
```

Platform specific dependency lists are in [docs/build/](docs/build/).

## Coding conventions

- C++17 only. Do not use C++20 features; RHEL 7/8 and older toolchains must keep compiling.
- Follow the existing naming: `snake_case` files, `PascalCase` types, `camelCase` methods.
- Keep public headers self-contained and add new headers so that the existing
  `file(GLOB_RECURSE ...)` install rules pick them up.
- Prefer `[[nodiscard]]`, `const`, `std::optional` and structured bindings where they
  clarify the API.
- Static analysis configuration lives in [`.clang-tidy`](.clang-tidy). Run it on changed
  translation units when possible.
- Do not reformat unrelated code; keep diffs reviewable.

## Commit messages

The history uses Conventional Commits style, for example:

```
feat(HTTP): expose URI path resolution helpers
fix(HTTP): correct realpath error handling in resolveLocalFilePathFromURI0NE
docs(build): update Windows build instructions
```

Use `feat`, `fix`, `refactor`, `perf`, `docs`, `build`, `ci` or `test` as type, and the
affected module as scope when it makes sense.

## Pull requests

1. Fork the repository and create a topic branch from `master`.
2. Add or update documentation when behavior changes.
3. Fill in the pull request template and link the related issues.
4. Make sure CI is green (Ubuntu 22.04/24.04/26.04, Debian 12/13, AlmaLinux 8/9/10).

Pull requests that break the supported platform matrix or the C++17 baseline will not be
merged.

## Reporting bugs

Use the GitHub issue templates. Include the version
(`pkg-config --modversion Mantids30_Helpers`), distribution, compiler, CMake version, the
exact commands you ran and the full error output.
