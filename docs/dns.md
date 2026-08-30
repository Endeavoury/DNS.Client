# DNS module

## Supported records

Typed parsing is implemented for A, AAAA, CNAME, MX, TXT, NS, PTR, SOA, SRV, NAPTR,
and CAA. Unknown types retain their numeric type, TTL, owner, section, exact raw RDATA,
and RFC 3597-style display text. The dispatch remains extensible for DS, DNSKEY, RRSIG,
NSEC/NSEC3, HTTPS/SVCB, TLSA, SSHFP, LOC, and other future records.

## Resolver and transport

`server = NULL` reads the platform resolver configuration (the first system resolver
for v1). Explicit server names and IPv4/IPv6 literals use `getaddrinfo`. Default port is
53 and default timeout is 5000 ms per transport. UDP is used first; a response carrying
the truncated flag is retried over RFC 1035 two-byte length-framed TCP. Query IDs and
echoed questions are validated.

## Reverse DNS

PTR input that parses as IPv4 is converted to reversed-octet `in-addr.arpa`. IPv6 is
converted to fully expanded, nibble-reversed `ip6.arpa`. An already constructed reverse
name is also accepted. The CLI auto-selects PTR for input that looks like an IP; the
explicit `ratos dns ptr` form is preferred in scripts.

## CLI output and exits

Plain output prints one answer RDATA value per line. JSON has stable top-level
`protocol`, `query`, and `response` objects. Every record includes `type`, `typeCode`,
`name`, `ttl`, `section`, and typed fields such as `address`, `exchange`, `strings`, or
`target` (unknown types use `data`). Additive JSON fields
may appear in minor versions; existing fields will not silently change meaning.
For reverse lookups, `query.name` is the user's IP input and `query.wireName` contains
the generated `in-addr.arpa` or `ip6.arpa` name.

| Exit | Meaning |
|---:|---|
| 0 | success / NOERROR |
| 1 | general failure |
| 2 | invalid arguments |
| 3 | timeout |
| 4 | network failure |
| 5 | DNS or protocol failure, including nonzero rcode |
| 6 | unsupported operation |
| 7 | permission denied |

## Security and limits

The parser bounds every read, limits records to 4096, packets to 65,535 bytes, names to
255 wire octets, labels to 63 octets, and compression traversal to 128 backward hops.
It rejects cycles, forward/out-of-packet pointers, malformed counts, invalid RDLENGTH,
trailing data, bad IDs, unexpected questions, and invalid reserved header bits.

## Current limitations

- DNSSEC records may be retained raw, but validation is not implemented.
- EDNS, encrypted DNS, mDNS, dynamic update, and AXFR are not in the query ABI.
- v1 uses one native attempt; the legacy .NET adapter supplies its existing resolver
  rotation/retry policy above the native call.
- Timeout cancellation is supported; native in-flight cancellation is not yet exposed.
