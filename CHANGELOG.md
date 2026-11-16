# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The current development series is `3.0.0`, defined by `SVERSION` in
`CMakeLists.txt`.

## [Unreleased]

### Fixed

- Restore GCC 8 / RHEL 8 compatibility for `DOUBLE::fromString` using
  locale-independent floating-point parsing without `std::from_chars`.

### Added

- GitHub Actions CI covering Ubuntu 24.04/26.04, Debian 12/13 and
  RHEL 8/9/10, building both shared and static libraries.
- Issue templates, pull request template and Dependabot configuration for
  GitHub Actions.
- `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md` and this changelog.
- Documentation index and rewritten build guides for Debian/Ubuntu,
  RHEL 7-10, Fedora and MSYS2/MinGW64.
- `docs/CLASSES.md` regenerated from the current source tree.

### Changed

- Removed redundant single-class namespaces: `JWT` now belongs to
  `Mantids30::DataFormat`, `FastRPC1` to `Mantids30::Protocol`, and `MantidsLang`
  to `Mantids30::Scripting`. Header paths and library target names are unchanged.
- `README.md` restructured: platform matrix, requirements, CMake options,
  installed layout and integration examples.

## [3.0.0]

### Added

- Parsing system documentation (`docs/PARSER.md`).
- `isAdmin()` support in the Monolith and RESTful client handlers.
- Admin bypass in the resource filter authorization chain.
- Public URI path resolution helpers in `HTTPv1_Server`.
- New `AppSpawn` contract and documentation.

### Changed

- Codebase modernized to C++17 (`[[nodiscard]]`, `std::optional`,
  structured bindings, `nullptr`, scoped enums, `override`).
- `AppSpawn` implementation modernized and bug fixed.
- `Filter::Action::toJSON()` and session query methods marked `[[nodiscard]]`.

### Fixed

- HTTP: `realpath` error handling in `resolveLocalFilePathFromURI0NE`.

[Unreleased]: https://github.com/unmanarc/libMantids30/compare/master...HEAD
