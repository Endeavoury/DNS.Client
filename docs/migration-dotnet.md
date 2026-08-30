# .NET compatibility and migration

The previous package exposed `DNS.Client.DnsClient`, `LookupClient`, `DnsMessage`, a
managed wire codec, cache helpers, custom-message send methods, and AXFR. Ratatoskr
keeps those public types while establishing `Ratatoskr` as the preferred namespace.

## Preserved behavior

- `DnsClient.Query/QueryAsync` and `LookupClient.Query/QueryAsync` route through C.
- UDP, TCP fallback, IPv4/IPv6 resolvers, timeout, retries, response errors, typed
  records, unknown RDATA, reverse DNS, and TTL caching remain available.
- Managed `DnsMessageCodec` remains for applications that construct/inspect packets.
- Existing record extension helpers and exception types remain.

## Intentional changes and temporary exceptions

- The compatibility project now targets .NET 8 instead of .NET Standard 2.0/2.1 so it
  can use the source-generated native binding. This is a breaking target-framework
  change; older runtimes should stay on the previous DNS.Client package.
- IPv6 reverse DNS is now accepted (previously it threw).
- Internet class (`IN`) is the only class accepted by the native query ABI.
- Infinite timeouts are not accepted for native queries; a positive bounded timeout is
  required so an unavailable resolver cannot hang indefinitely.
- `SendAsync`, `SendTcpAsync`, and `TransferZoneAsync` retain their managed legacy
  implementations for source compatibility. AXFR/custom packets need a future native
  message/stream ABI before they can obey the one-implementation rule, so new code
  should not depend on these compatibility-only paths.
- Managed cancellation cannot interrupt an already running synchronous native call;
  it is observed before and after it. The configured native timeout remains enforced.

## Preferred API

```csharp
using Ratatoskr;

var client = new DnsClient(new DnsClientOptions { Server = "1.1.1.1" });
DnsResult result = await client.QueryAsync("example.com", DnsRecordType.A);
```
