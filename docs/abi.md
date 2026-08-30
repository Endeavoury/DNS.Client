# C ABI contract

## Identity and versions

Every public symbol begins with `ratos_`. `RATOS_ABI_VERSION` and
`ratos_abi_version()` report the native ABI generation; product versions come from
`ratos_version_major/minor/patch()`. Public headers are C++ compatible via `extern "C"`.

## Ownership

- `ratos_context_create` returns an owned context; call `ratos_context_destroy`.
- Successful `ratos_dns_query` returns an owned result; call
  `ratos_dns_result_destroy`.
- Records returned by `ratos_dns_result_record` are borrowed from the result.
- Strings and raw bytes returned by accessors are borrowed and immutable. They remain
  valid until the owning context/result is changed or destroyed as documented.
- Callers never pass Ratatoskr memory to the platform C runtime's `free`.
- Destroy functions accept `NULL`.

## Errors

Functions return stable `ratos_error` values. `ratos_error_string` returns a permanent
static description. `ratos_context_error` adds operation detail and remains valid until
the next operation on that context or context destruction. The library does not print.

DNS response rcodes are valid structured responses: `ratos_dns_query` can return
`RATOS_OK` while `ratos_dns_result_rcode` reports NXDOMAIN, SERVFAIL, or REFUSED. This
lets SDK callers inspect authority records and the response code. The CLI maps nonzero
rcodes to exit code 5.

## ABI-safe types

Owned types are opaque. ABI fields use `uint8_t`, `uint16_t`, `uint32_t`, or pointers.
`ratos_dns_type` and `ratos_dns_section` are fixed-width typedefs with named constants,
not compiler-sized enum fields. Query options must be initialized with
`ratos_dns_query_options_init`; `struct_size` protects appended-field evolution.

## Compatibility policy

Within ABI 1 Ratatoskr may add symbols, constants, record types, and fields appended to
versioned input structs. It will not remove/rename symbols, reorder existing fields,
change ownership, or expose private layouts. A binary-incompatible change increments
the ABI version and shared-library major version.

