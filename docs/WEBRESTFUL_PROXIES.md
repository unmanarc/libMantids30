# Ordered conditional proxies

`Proxies` entries are evaluated in their configuration order, including repeated
paths. The first entry whose path and optional `MatchExpression` match handles
the request. Each entry defines its own upstream host, port, TLS and headers.

```text
Proxies
{
    /portal
    {
        MatchExpression "IS_EQUAL($session.roles,\"admin\")"
        RemoteHost "127.0.0.1"
        RemotePort 9001
        UseTLS false
    }
    /portal
    {
        MatchExpression "IS_EQUAL($session.scopes,\"READ\")"
        RemoteHost "127.0.0.1"
        RemotePort 9002
        UseTLS false
    }
    /portal
    {
        RemoteHost "127.0.0.1"
        RemotePort 9003
        UseTLS false
    }
}
```

The example uses Boost Property Tree INFO syntax. Quotes inside expressions must
be escaped. Place specific rules before broader rules. A missing expression means
unconditional matching; an explicitly empty or syntactically invalid expression
rejects server creation. Evaluation errors return HTTP 500 and do not select
a later rule. If every rule fails to match, normal file handling applies. To
restrict a resource, ensure that later rules and local files do not expose it.

A mount `/portal` matches `/portal`, `/portal/` and `/portal/...`, but not
`/portal-other`. The mount prefix is removed before forwarding; an exact mount
request forwards `/`. A root mount `/` (or an empty path) retains the full URI.
API routing and configured redirections still take precedence over proxies.

Expressions use the existing JSONExprEval syntax: `IS_EQUAL`, `CONTAINS`,
`STARTS_WITH`, `ENDS_WITH`, `REGEX_MATCH`, `IS_NULL`, `!`, ` && ` and ` || `.
Use parentheses when combining AND and OR. `IS_EQUAL` against an array tests
membership, so it is appropriate for exact roles and scopes. For example:

```text
MatchExpression "IS_EQUAL($session.user,\"alice\")"
MatchExpression "IS_EQUAL($session.roles,\"admin\") || IS_EQUAL($session.scopes,\"READ\")"
MatchExpression "IS_EQUAL($client.ip,\"192.0.2.4\") && IS_EQUAL($request.get.target,\"blue\")"
```

The context is built once per request, after session validation, only if a
matching route has an expression:

| JSON path | Value |
| --- | --- |
| `session.isActive` | Whether authentication succeeded |
| `session.isAdmin` | Whether the authenticated session is an administrator |
| `session.roles`, `session.scopes` | Arrays of validated roles/scopes |
| `session.user`, `session.domain` | Authenticated user and domain |
| `session.isImpersonation`, `session.impersonator` | Authenticated impersonation context |
| `session.claims` | Validated session JWT claims |
| `client.ip` | Client IP reported by the HTTP connection |
| `client.tlsCN`, `client.isSecure` | TLS common name and connection security |
| `client.userAgent` | User agent |
| `request.method`, `request.uri` | HTTP method and path without query |
| `request.query` | Raw query string |
| `request.json` | Parsed JSON body, or null when unavailable/invalid |
| `request.get`, `request.post` | Decoded query/form fields; repeated values are arrays |
| `request.headers` | Raw header values keyed by uppercase names; repeated headers are arrays |
| `request.origin`, `request.referer` | Origin and Referer |

For anonymous or invalid credentials, `session.isActive` and `session.isAdmin`
are false, roles/scopes are empty, and user/domain/claims/impersonation fields
are absent. Header and query data are client supplied; authentication conditions
should use `session` fields. Compare booleans as strings, e.g.
`IS_EQUAL($session.isActive,"true")`.

Programmatic route registration now uses an ordered vector instead of a map:

```cpp
auto condition = std::make_shared<RequestMatchExpression>("IS_EQUAL($session.roles,\"admin\")");
if (!condition->isCompiled())
{
    return false;
}
config.dynamicRequestHandlersByRoute.emplace_back(
    "/portal", APIServerConfig::DynamicRequestHandlerDef{handler, parameters, condition});
```

Configure routes before serving requests. Compiled expressions are shared and
serialize evaluation because JSONExprEval maintains lazy caches.

`RequestMatchExpression::evaluate(context, matched, &error)` returns success or
failure separately from the boolean condition result. Invalid compilation and
regular expressions are reported through return values and error text.
