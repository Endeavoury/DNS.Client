# Legacy .NET compatibility API reference

These types remain for source migration. The canonical interoperability contract is
the C ABI in [abi.md](abi.md), and new .NET code should prefer `namespace Ratatoskr`.

## High-level API

`LookupClient` is the recommended application entry point. It discovers local name
servers by default or accepts `LookupClientOptions.NameServers`. `Query` is the
blocking form; `QueryAsync` accepts a cancellation token. `ClearCache` removes all
cached responses.

## Low-level API

`DnsClient` exposes `Query`, `QueryAsync`, `SendAsync`, `SendTcpAsync`,
`QueryReverseAsync`, and `TransferZoneAsync`. Set `Timeout`, `Attempts`, and
`ThrowOnResponseError` to control operational behavior.

`DnsMessage.CreateQuery` creates a conventional recursive query. `ToArray` and
`Parse` are the wire-format boundary and are useful for packet captures, fixtures,
and custom protocol tooling.

## Exceptions

- `TimeoutException`: the configured operation timeout elapsed.
- `OperationCanceledException`: the caller canceled the operation.
- `DnsProtocolException`: malformed, truncated, or response-mismatched data.
- `DnsResponseException`: optional exception for a non-zero DNS response code.

## Threading

Client instances are safe to use concurrently for independent queries. Configuration
properties should be set before starting concurrent work. `LookupClient`'s cache is
concurrent and can be cleared while queries are running.
