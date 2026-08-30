# DNS.Client

[![Build and test](https://github.com/Endeavoury/DNS.Client/actions/workflows/ci.yml/badge.svg)](https://github.com/Endeavoury/DNS.Client/actions/workflows/ci.yml)
[![Release](https://github.com/Endeavoury/DNS.Client/actions/workflows/package.yml/badge.svg)](https://github.com/Endeavoury/DNS.Client/actions/workflows/package.yml)
[![NuGet](https://img.shields.io/nuget/v/Endeavoury.DNS.Client.svg)](https://www.nuget.org/packages/Endeavoury.DNS.Client)
[![GitHub release](https://img.shields.io/github/v/release/Endeavoury/DNS.Client)](https://github.com/Endeavoury/DNS.Client/releases)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

`DNS.Client` is a dependency-free, wire-compatible DNS resolver library for .NET.
It implements the client-side protocol described by [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035),
with a modern high-level API for application lookups and a low-level API for protocol work.

## Why this package?

- Works on every runtime implementing **.NET Standard 2.1**.
- Uses UDP by default, retries across resolvers, and falls back to TCP for truncated responses.
- Preserves unknown record types as raw RDATA instead of discarding bytes.
- Supports synchronous and asynchronous APIs, cancellation, timeouts, AXFR, and reverse DNS.
- Includes a TTL-aware `LookupClient` cache and typed record collection helpers.
- Has no third-party runtime dependencies.

## Install

```bash
dotnet add package Endeavoury.DNS.Client
```

## A first lookup

```csharp
using System.Net;
using DNS.Client;

var lookup = new LookupClient();
var response = await lookup.QueryAsync("example.com", QuestionType.A);

foreach (var address in response.Answers.ARecords())
    Console.WriteLine(address.Address);
```

For deterministic infrastructure or tests, specify resolvers explicitly:

```csharp
var lookup = new LookupClient(new LookupClientOptions
{
    NameServers = new[] { new NameServer(IPAddress.Parse("1.1.1.1")) },
    Timeout = TimeSpan.FromSeconds(2),
    Retries = 3,
    EnableCache = true
});
```

## Capabilities at a glance

| Area | Included |
| --- | --- |
| Queries | A, AAAA, NS, CNAME, MX, PTR, SOA, TXT, SRV, NAPTR, CAA, HINFO, MINFO, WKS and more |
| Transport | UDP, TCP framing, truncation fallback, retries, cancellation |
| Message format | Header flags, all four sections, name compression, pointer-loop protection |
| Resolver features | Server discovery, multiple endpoints, TTL cache, reverse lookup, AXFR |
| Extensibility | Unknown QTYPE/CLASS and RDATA are retained losslessly |
| Tooling | .NET Standard 2.1 library, tests, console sample, GitHub Actions releases |

## Choose the right API

| Need | API |
| --- | --- |
| Normal application lookup | `LookupClient.QueryAsync` |
| A specific resolver/port | `LookupClientOptions.NameServers` or `DnsClient(IPEndPoint[])` |
| Custom opcode, sections, or raw records | `DnsMessage` + `DnsMessageCodec` |
| Zone transfer | `TransferZoneAsync` |
| Inspect every byte | `DnsMessage.Parse` / `DnsMessage.ToArray` |

## Documentation

- [Usage guide](docs/usage.md) — queries, options, records, cancellation, and examples.
- [DNS fundamentals](docs/dns-fundamentals.md) — names, records, recursion, caching, and transport.
- [Technical architecture](docs/architecture.md) — wire codec, resolver pipeline, and failure handling.
- [Record reference](docs/records.md) — supported types, models, and unknown-record behavior.
- [API reference](docs/api-reference.md) — public types and recommended usage patterns.
- [Troubleshooting](docs/troubleshooting.md) — timeouts, truncation, NXDOMAIN, and package diagnostics.
- [Contributing](docs/contributing.md) — local development, tests, style, and pull requests.
- [Publishing](docs/publishing.md) — semantic versions, GitHub Releases, and Trusted Publishing.

## Project layout

```text
DNS.Client/             Library source
DNS.Client.Tests/       Protocol and transport tests
DNS.Client.Console/     Small command-line example
docs/                   Developer and protocol documentation
.github/workflows/      Build, test, release, and package publishing
```

## Compatibility and security

The library targets .NET Standard 2.1 and does not execute code received from DNS
servers. DNS answers are untrusted input: validate names and record content before
using them in security-sensitive decisions. Classic DNS provides no confidentiality
or authenticity; use a trusted transport or DNSSEC-aware infrastructure when those
properties are required.

## License

Released under the [MIT License](LICENSE).
