# DNS.Client

`DNS.Client` is a dependency-free .NET Standard 2.1 DNS client and wire-format library for the
client-side protocol defined by [RFC 1035](https://www.rfc-editor.org/info/rfc1035/).

It supports:

- standard, inverse, and status message opcodes;
- all RFC 1035 TYPE, QTYPE, CLASS, QCLASS, and response-code values;
- the A, CNAME, HINFO, MB, MD, MF, MG, MINFO, MR, MX, NULL, NS, PTR, SOA, TXT,
  and WKS RDATA formats, with lossless raw data for unknown extension types;
- all four DNS sections and correct header counts;
- reading and writing compressed names, including pointer safety and RFC name limits;
- UDP retries, multiple resolvers, response matching, the RFC 512-byte UDP limit,
  and automatic TCP retry for truncated responses;
- explicit TCP queries and multi-message AXFR zone transfers;
- IPv4 reverse lookup through the RFC 1035 IN-ADDR.ARPA namespace;
- synchronous and asynchronous calls, cancellation, and configurable timeouts.

The package is a DNS client, not an authoritative name server or a zone-file database.
RFC 1035's server architecture and master-file storage sections are therefore outside
its public surface.

## Install

The package is published to GitHub Packages and nuget.org. Configure the owner feed
for GitHub Packages, or install directly from nuget.org:

```bash
dotnet nuget add source \
  --username YOUR_GITHUB_USERNAME \
  --password YOUR_GITHUB_TOKEN \
  --store-password-in-clear-text \
  --name github-roygerritse \
  https://nuget.pkg.github.com/RoyGerritse/index.json

dotnet add package DNS.Client --source github-roygerritse
```

The token needs `read:packages`. Keep it outside committed configuration files.

From nuget.org, use the standard source:

```bash
dotnet add package DNS.Client --source https://api.nuget.org/v3/index.json
```

## Query a resolver

```csharp
using DNS.Client;

var client = new DnsClient(); // discovers DNS servers from active interfaces
DnsMessage response = await client.QueryAsync("example.com", QuestionType.A);

foreach (DnsResourceRecord answer in response.Answers)
{
    if (answer.Data is ARecordData a)
        Console.WriteLine($"{answer.Name} -> {a.Address} (TTL {answer.TimeToLive})");
}
```

An explicit resolver, non-standard port, or fallback list can be supplied:

```csharp
var one = new DnsClient(IPAddress.Parse("192.0.2.53"));
var several = new DnsClient(new[]
{
    new IPEndPoint(IPAddress.Parse("192.0.2.53"), 53),
    new IPEndPoint(IPAddress.Parse("198.51.100.53"), 53)
});
several.Timeout = TimeSpan.FromSeconds(2);
several.Attempts = 4;
```

By default, DNS error responses are returned so callers can inspect authority records
and `Header.ResponseCode`. Set `ThrowOnResponseError = true` to receive a
`DnsResponseException` instead.

## Build a message or parse bytes

```csharp
var query = DnsMessage.CreateQuery(0x1234, "example.com", QuestionType.MX);
byte[] wire = query.ToArray();
DnsMessage parsed = DnsMessage.Parse(wire);
```

For lower-level operations, create a `DnsMessage`, set its `DnsHeader`, and add
questions or records to any section. `SendAsync` uses UDP with automatic TCP fallback;
`SendTcpAsync` always uses TCP. `TransferZoneAsync` performs an AXFR over TCP.

See the [package guide](https://github.com/RoyGerritse/DNS.Client/blob/master/docs/usage.md)
for record models, error handling, low-level messages, reverse queries, and zone
transfers. Maintainers can find the tag-based release process in the
[publishing documentation](https://github.com/RoyGerritse/DNS.Client/blob/master/docs/publishing.md).

Pushes to `master` publish CI prereleases; `v*` tags publish stable versions to both
GitHub Packages and nuget.org.

## Verification

```bash
dotnet test DNS.Client.sln
```
