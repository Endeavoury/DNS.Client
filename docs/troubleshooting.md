# Troubleshooting

## Every query times out

Check that UDP/TCP port 53 is reachable from the process, that the configured
endpoint is correct, and that a local firewall or VPN is not intercepting DNS.
Pass a known resolver explicitly to distinguish discovery problems from transport
problems.

## The response is `NameError`

`NameError` is the DNS name commonly called NXDOMAIN. It is a valid DNS response,
not a packet failure. Inspect `Header.ResponseCode` and authority records, or enable
throwing behavior if your application treats it as exceptional.

## Answers are missing

Follow `CNAME` records, inspect all four sections, and check whether the response is
truncated. Large DNSSEC or TXT responses require TCP fallback; keep `TC=1` handling
enabled.

## AXFR is refused

Authoritative servers normally restrict zone transfers by address or TSIG policy.
`TransferZoneAsync` cannot bypass that policy; request access from the zone operator.

## Debugging bytes

Capture `DnsMessage.ToArray(compressNames: false)` and compare it with a known-good
packet. `DnsProtocolException` messages identify the section and length boundary
where parsing stopped. Avoid logging secrets or full DNS payloads in production.
