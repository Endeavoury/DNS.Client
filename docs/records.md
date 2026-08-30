# DNS record reference

| Type | Model | Important fields |
| --- | --- | --- |
| A | `ARecordData` | IPv4 `Address` |
| AAAA | `AaaaRecordData` | IPv6 `Address` |
| NS, CNAME, PTR, MB, MD, MF, MG, MR, DNAME | `NameRecordData` | Target `Name` |
| MX | `MxRecordData` | `Preference`, `Exchange` |
| SRV | `SrvRecordData` | `Priority`, `Weight`, `Port`, `Target` |
| SOA | `SoaRecordData` | server, mailbox, serial, refresh, retry, expire, minimum |
| HINFO | `HInfoRecordData` | CPU and operating system |
| MINFO | `MInfoRecordData` | responsible and error mailbox |
| TXT | `TxtRecordData` | one or more `Strings` |
| WKS | `WksRecordData` | address, protocol, service bitmap |
| NAPTR | `NAPTRRecordData` | order, preference, flags, services, regexp, replacement |
| CAA | `CaaRecordData` | flags, tag, value |
| NULL and unknown | `RawRecordData` | opaque bytes |

Collection helpers are available on answer lists:
`ARecords()`, `AaaaRecords()`, `MxRecords()`, `SrvRecords()`, and `TxtRecords()`.
For less common types, filter by `record.Type` and inspect `record.Data`.

Record names and text use the package's lossless wire encoding. Do not assume that
all TXT values are UTF-8; DNS character-strings are byte sequences.
