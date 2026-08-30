# DNS RFC compliance matrix

This historical audit describes the preserved managed compatibility surface; the
canonical native implementation is documented in [dns.md](dns.md). `COMPLETE` means the applicable behavior is implemented and covered by
tests; `PARTIAL` means the basic behavior exists but one or more current requirements
remain; `MISSING` means no implementation exists. Statuses are deliberately scoped to
this package's role as a stub/client library, not an authoritative or recursive server.

## Repository architecture audit

- **Packet encoder/decoder:** `bindings/dotnet/src/Ratatoskr.Compatibility/Protocol/DnsMessageCodec.cs` and `DnsWire.cs`.
- **Message model:** `DnsProtocol.cs`, `Question/*`, and `Resource/DnsResourceRecord.cs`.
- **Transport:** `DnsClient.cs` (UDP, TCP framing, truncation fallback, AXFR).
- **Resolver policy:** `bindings/dotnet/src/Ratatoskr.Compatibility/LookupClient.cs` (server selection and TTL cache).
- **Cache:** in-memory positive cache in `LookupClient`; no negative/stale tiers yet.
- **Authoritative, recursive, DNSSEC, EDNS, DoT/DoH/DoQ, mDNS/DNS-SD:** not present.
- **Tests:** native parser/builder tests and fuzz targets under `tests/dns` and `fuzz/dns`,
  plus managed compatibility tests under `bindings/dotnet/tests/Ratatoskr.Compatibility.Tests`.

## Security findings from the audit

1. Header AD/CD bits were not modeled separately from the RFC 1035 Z bit (fixed in Phase 1).
2. The decoder had no configurable packet/record-count budget before section allocation (fixed in Phase 1).
3. EDNS extended RCODE and OPT data were unavailable, so BADVERS could not be represented.
4. The cache did not distinguish positive, NXDOMAIN, NODATA, transient failure, or stale data.
5. TCP connections are per-query; connection reuse and idle limits are not implemented.
6. Response matching validates ID, opcode, and question but does not yet expose source/timing metadata.

Phase 1 begins with header correctness, RFC 9619 QDCOUNT validation, and bounded decoding;
each change is covered by focused malformed-input or wire-format tests.

| RFC | Feature | Applicable to this client? | Status | Code location | Tests | Notes |
| --- | ------- | -------------------------- | ------ | ------------- | ----- | ----- |
| 1034 | DNS concepts, names, delegation | Yes | PARTIAL | `Protocol/DnsWire.cs`, `DnsClient.cs` | `MessageCodecTests` | Name and stub-query behavior exist; no iterative delegation. |
| 1035 | Classic DNS message and transport | Yes | PARTIAL | `Protocol/*`, `DnsClient.cs` | `MessageCodecTests`, `TransportTests` | Core codec/UDP/TCP/AXFR exist; header and modern update gaps remain. |
| 1123 | Host DNS resolver requirements | Yes | PARTIAL | `DnsClient.cs`, `bindings/dotnet/src/Ratatoskr.Compatibility/LookupClient.cs` | `TransportTests` | Bounded retries and cancellation exist; resolver policy needs richer error classes. |
| 1982 | Serial number arithmetic | AXFR/zone consumers only | MISSING | — | — | Needed to compare SOA serials safely. |
| 2181 | DNS clarifications and RR TTL/class rules | Yes | PARTIAL | `DnsMessageCodec.cs`, `DnsResourceRecord.cs` | codec tests | TTL and class are parsed; RRset and canonical validation are incomplete. |
| 2308 | Negative caching | Yes | MISSING | `bindings/dotnet/src/Ratatoskr.Compatibility/LookupClient.cs` | — | Current cache is positive-answer-only and must not cache failures accidentally. |
| 3597 | Unknown RR type handling | Yes | COMPLETE | `DnsMessageCodec.cs`, `RawRecordData` | round-trip tests | Unknown TYPE/RDATA is retained as opaque bytes. |
| 4343 | DNS case insensitivity | Yes | PARTIAL | `DnsWireWriter`, response validation | — | Comparison is case-insensitive; canonical presentation/normalization needs tests. |
| 4592 | Wildcard semantics | Stub only | NOT_APPLICABLE | — | — | Wildcard synthesis belongs to authoritative/recursive resolution. |
| 5452 | Response spoofing resistance | Yes | PARTIAL | `DnsClient.ValidateResponse` | transport tests | ID/question/source association exists; entropy and source validation need hardening. |
| 5936 | AXFR | Yes | PARTIAL | `DnsClient.TransferZoneAsync` | — | Multi-message transfer and SOA termination exist; size/serial validation is incomplete. |
| 6891 | EDNS(0) | Yes | MISSING | — | — | No OPT pseudo-RR, payload negotiation, DO bit, or extended RCODE. |
| 6895 | DNS IANA registries/extended codes | Yes | PARTIAL | `QuestionType`, `QuestionClass`, `DnsResponseCode` | — | Basic enums exist; extended RCODE and unknown values need explicit modeling. |
| 7766 | DNS over TCP | Yes | PARTIAL | `DnsClient.cs` | `TransportTests` | Framing and partial reads work; connection reuse/idle policy is absent. |
| 7871 | EDNS Client Subnet | Optional | MISSING | — | — | Requires generic EDNS option support first. |
| 7873 | DNS Cookies | Optional | MISSING | — | — | Requires EDNS option model and resolver policy. |
| 8020 | NXDOMAIN cut | Recursive only | NOT_APPLICABLE | — | — | This package does not perform iterative recursion. |
| 8198 | Aggressive DNSSEC caching | Recursive/validator only | NOT_APPLICABLE | — | — | No local DNSSEC validator or recursive resolver. |
| 8767 | Serve stale | Optional | MISSING | `bindings/dotnet/src/Ratatoskr.Compatibility/LookupClient.cs` | — | Cache policy does not distinguish stale data. |
| 8914 | Extended DNS Errors | Yes | MISSING | — | — | EDE option and exposure are absent. |
| 9156 | QNAME minimization | Recursive only | NOT_APPLICABLE | — | — | Explicitly a stub resolver; no iterative name-server walk. |
| 9499 | DNS terminology | Yes | PARTIAL | docs | — | Terminology is used inconsistently and will be normalized in API docs. |
| 9520 | DNS privacy considerations | Yes | PARTIAL | `DnsClient.cs`, docs | — | No encrypted transport; logging defaults are minimal. |
| 9619 | QDCOUNT behavior | Yes | MISSING | `DnsMessageCodec.cs` | — | Current parser accepts arbitrary QDCOUNT; current interoperability guidance needs explicit policy. |
| 1876 | LOC record | Optional | MISSING | — | — | Record model/codec not present. |
| 2782 | SRV record | Yes | PARTIAL | `Resource/DnsResourceRecord.cs`, codec | — | Basic SRV model exists; service-selection policy is absent. |
| 3403 | NAPTR record | Yes | PARTIAL | `Resource/DnsResourceRecord.cs`, codec | — | Basic fields exist; escaped regexp/service semantics need validation. |
| 3596 | IPv6 AAAA record | Yes | COMPLETE | `AaaaRecordData`, codec | — | Fixed-length address validation and codec support exist. |
| 6763 | DNS-SD presentation | Optional | MISSING | — | — | No service enumeration/presentation layer. |
| 9460 | SVCB/HTTPS | Yes for modern clients | MISSING | — | — | Generic SvcParam handling is absent. |
| 4033 | DNSSEC concepts | Optional | MISSING | — | — | No validation subsystem. |
| 4034 | DNSSEC RR formats | Optional | MISSING | — | — | DS/DNSKEY/RRSIG/NSEC models absent. |
| 4035 | DNSSEC protocol behavior | Optional | MISSING | — | — | No chain validation. |
| 5011 | Automated trust-anchor maintenance | Optional | NOT_APPLICABLE | — | — | No validator/trust-anchor store. |
| 5155 | NSEC3 | Optional | MISSING | — | — | No parser or validator. |
| 5702 | DNSSEC RSA/SHA-256 | Optional | MISSING | — | — | Algorithms intentionally not added without validator scope. |
| 6605 | DNSSEC GOST | Optional | SUPERSEDED | — | — | Obsolete algorithm; current DNSSEC requirements take precedence. |
| 6840 | DNSSEC clarifications | Optional | MISSING | — | — | Depends on DNSSEC subsystem. |
| 7344 | CDS/CDNSKEY bootstrapping | Optional | MISSING | — | — | No DNSSEC provisioning support. |
| 7583 | DNSSEC key rollover timing | Optional | NOT_APPLICABLE | — | — | Validator/key-management scope absent. |
| 8078 | CDS/CDNSKEY updates | Optional | NOT_APPLICABLE | — | — | Authoritative provisioning is out of scope. |
| 8080 | Ed25519 DNSSEC | Optional | MISSING | — | — | Requires validator and current algorithm policy. |
| 9077 | DNSSEC algorithm guidance | Optional | MISSING | — | — | Track when validator work begins. |
| 9364 | DNSSEC algorithm requirements | Optional | MISSING | — | — | Track current required algorithms; do not revive deprecated ones. |
| 1995 | IXFR | Optional | MISSING | — | — | Zone synchronization is not currently exposed. |
| 1996 | NOTIFY | Optional | NOT_APPLICABLE | — | — | No authoritative administration API. |
| 2136 | Dynamic UPDATE | Optional | NOT_APPLICABLE | — | — | Query-only client; no UPDATE model. |
| 2930 | TKEY | Optional | NOT_APPLICABLE | — | — | No authenticated transaction API. |
| 8945 | TSIG | Optional | NOT_APPLICABLE | — | — | Authentication is outside current scope; do not implement obsolete variants. |
| 6761 | Special-use names | Yes | PARTIAL | `LookupClient` | — | No policy table currently prevents public resolution of special-use names. |
| 6762 | Multicast DNS | Optional | NOT_APPLICABLE | — | — | Unicast resolver only; no multicast socket behavior. |
| 7626 | DNS privacy analysis | Yes | PARTIAL | docs | — | Documents plaintext limitation; DoT/DoH not implemented. |
| 7858 | DNS over TLS | Optional | MISSING | — | — | No TLS transport. |
| 8094 | DNS over DTLS | Optional | MISSING | — | — | No DTLS transport. |
| 8310 | TLS resolver profiles | Optional | MISSING | — | — | Depends on DoT implementation. |
| 8467 | Padding policy | Optional | MISSING | — | — | Depends on EDNS/encrypted transport. |
| 8484 | DNS over HTTPS | Optional | MISSING | — | — | No HTTP transport. |
| 8490 | DNS over HTTPS usage | Optional | MISSING | — | — | Depends on DoH implementation. |
| 8906 | DoT/DoH operational guidance | Optional | MISSING | — | — | Depends on encrypted transports. |
| 9103 | DoH/DoT deployment guidance | Optional | MISSING | — | — | No encrypted transport. |
| 9230 | Oblivious DoH | Optional | NOT_APPLICABLE | — | — | No ODoH scope. |
| 9250 | DNS over QUIC | Optional | MISSING | — | — | No QUIC dependency or transport. |
| 9461 | Discovery of design-specific DNS servers | Optional | MISSING | — | — | No encrypted resolver discovery. |
| 9462 | Discovery of designated resolvers | Optional | MISSING | — | — | No resolver discovery protocol beyond OS configuration. |

## Phase plan

1. Harden the packet layer: header bits, QDCOUNT policy, strict limits, and fuzzable APIs.
2. Complete core RR parsing and modern error/response metadata.
3. Add EDNS(0), generic options, extended RCODE, and EDE.
4. Correct positive/negative/stale cache policy.
5. Add optional SVCB/HTTPS and DNSSEC record parsing, then validation only with a separate trust model.
6. Add encrypted transports behind a transport interface if dependency and API scope remain justified.

Every phase must add golden-wire, malformed-input, and regression tests and update this matrix.
