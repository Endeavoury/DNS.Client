# DNS fundamentals

## The naming system

DNS maps names to data in a hierarchical tree. A fully qualified name is a sequence
of labels ending at the root (`.`), for example `www.example.com.`. Labels are at
most 63 octets and a complete wire name is at most 255 octets. The package accepts
the familiar trailing-dot form and normalizes names only when comparing responses.

## Records and zones

A resource record has an owner name, type, class, TTL, and RDATA. A zone is an
administrative portion of the tree served by an authoritative server. Common records:

- `A` and `AAAA` map names to IPv4 and IPv6 addresses.
- `NS` delegates a zone; `SOA` describes its authority and serial number.
- `CNAME` aliases one name to another.
- `MX` identifies mail exchangers.
- `TXT`, `SRV`, `NAPTR`, and `CAA` carry application/service metadata.
- `PTR` maps an address name back to a hostname.

The `QuestionType` and `QuestionClass` enums cover standard values. Unknown numeric
values are still parsed and serialized, with their RDATA exposed as `RawRecordData`.

## Recursive resolution

An application normally asks a recursive resolver. The resolver follows referrals
from the root, TLD, and authoritative servers, then returns an answer and its TTL.
`DnsClient` is a stub client: it sends the query to configured recursive servers and
does not implement the full server-side iterative algorithm.

## UDP, TCP, and truncation

Classic DNS uses UDP port 53 for small messages. RFC 1035 limits ordinary UDP DNS
messages to 512 octets. A response with `TC=1` is incomplete; the client retries the
same query over TCP, where each message is prefixed by a two-octet length. AXFR is
TCP-only and consists of multiple messages terminated by the second SOA record.

## Caching and negative answers

`LookupClient` caches successful responses until the shortest answer TTL expires.
`NXDOMAIN` and other response codes remain visible to callers by default. Set
`ThrowDnsErrors` (or `DnsClient.ThrowOnResponseError`) when an application prefers
exceptions over inspecting `DnsResponseCode`.

## Reverse DNS

IPv4 reverse names reverse the octets below `in-addr.arpa`: `192.0.2.10` becomes
`10.2.0.192.in-addr.arpa`. `QueryReverseAsync` creates this PTR query. IPv6 reverse
names use the `ip6.arpa` nibble format and can be queried directly as a normal name.
