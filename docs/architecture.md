# Ratatoskr architecture

## One canonical implementation

Protocol behavior lives in a portable C11 core. C was selected as the interoperability
boundary because its ABI is directly consumable from the major language runtimes and
does not impose a runtime, exception model, object layout, or package manager on users.
Bindings translate language-native inputs/results but do not reimplement DNS and do not
execute the CLI.

```text
 C / .NET / CLI  ---> stable ratos_* ABI ---> portable C core
                                                   |
                                      src/protocols/dns (first)
```

The library never depends on the CLI. The CLI and bindings are peer consumers.

## Layers

- `src/core`: context, errors, allocation helpers, versions, and isolated platform code.
- `src/protocols/dns`: high-level query flow, packet builder, hostile-input parser,
  UDP transport, and TCP transport. Each concern has a separate translation unit.
- `include/ratatoskr`: the only public native contract.
- `cli`: argument parsing and rendering only.
- `bindings`: ownership-safe, language-idiomatic wrappers.
- `cmake`: shared compiler, option, and install policy; component source lists stay
  beside the component that owns them.

A future protocol gets `src/protocols/<name>`, `include/ratatoskr/<name>.h`, and an
optional `cli/commands/<name>.c`. Ratatoskr deliberately has no inheritance-like
generic protocol framework; normal sockets and small composable helpers are enough
until real shared requirements emerge.

The Java binding targets Java 22 or newer and uses the finalized Foreign Function &
Memory API. It creates a context per query, copies native results into immutable Java
values, and destroys native ownership before returning. Published Maven artifacts may
bundle platform libraries, but those files are builds of the same canonical C source.

The Python binding targets Python 3.11 or newer and uses only standard-library
`ctypes` and `asyncio`. Native handles never enter the public API: each query creates a
context, copies a result into frozen Python values, and releases all native ownership.
Its platform wheels each contain one matching build of the canonical C library; the
source distribution uses an explicitly selected or system-installed library.

## Result and ABI design

Contexts and DNS results are opaque. Records are borrowed opaque views owned by their
result. Accessors expose fixed-width scalars, borrowed immutable strings, and raw RDATA.
Indexed typed fields allow new record types without expanding a public union or changing
record size. Unknown records retain their numeric type and exact RDATA.

`ratos_dns_query_options` starts with `struct_size`, allowing future callers and the
library to negotiate appended fields. Its ABI-visible values use fixed-width integer
types. Every Ratatoskr allocation has a typed destroy operation.

## Query flow

1. Apply defaults and select the requested or OS-configured resolver.
2. Convert IP input for PTR to `in-addr.arpa` or nibble-reversed `ip6.arpa`.
3. Build one RFC 1035 question with a randomized transaction ID.
4. Exchange through a connected IPv4/IPv6 UDP socket under a timeout.
5. Strictly parse and validate ID, flags, echoed question, names, counts, and RDATA.
6. On `TC=1`, retry the same query through length-framed TCP.
7. Return an immutable structured result.

## Threading and async

Results are immutable after construction and may be read concurrently. Separate
contexts can be used concurrently. A single context is not currently safe for
simultaneous operations because its last-error buffer is mutable. No resolver state is
global. The native v1 call is synchronous; managed bindings use worker scheduling.
Native polling/cancellation tasks can be added later without changing the sync API.

## Compatibility

ABI version changes only for binary-incompatible changes. Functions and accessors can
be appended within ABI 1. Struct layouts are never exposed for owned objects. The
product uses semantic versions independently from the ABI version. See [abi.md](abi.md).
Repository placement and dependency rules are documented in
[repository-layout.md](repository-layout.md).
