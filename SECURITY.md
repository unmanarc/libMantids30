# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 3.0.x | Yes |
| < 3.0 | No |

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Use one of the following private channels:

1. GitHub private advisory:
   <https://github.com/unmanarc/libMantids30/security/advisories/new>
2. Email: <dev@unmanarc.com> (PGP available on request)

Include:

- Affected version or commit.
- Affected module (for example `Protocol_HTTP`, `DataFormat_JWT`, `DB_*`).
- A description of the impact and, when possible, a minimal reproducer.
- Your preferred attribution, if any.

## What to expect

- Acknowledgement of the report within 7 days.
- An assessment and, when applicable, a fix in a private branch.
- A coordinated disclosure date agreed with the reporter.

## Scope

The following areas are considered security relevant:

- TLS handling in `Net_Sockets`, `Net_Chains` and the HTTP server.
- JWT generation, validation and revocation in `DataFormat_JWT` and `Config_JWT`.
- Session and authentication handling in `API_EndpointsAndSessions` and the web servers.
- Parsing code reachable from untrusted network input.
- Cryptographic helpers (`Helpers::Crypto`, `Helpers::OTP`, `Helpers::Random`).

Issues that require an already compromised host, or that only affect unsupported
distributions, are out of scope but still welcome as regular bug reports.
