# Ratatoskr

[![Build and test](https://github.com/Endeavoury/Ratatoskr/actions/workflows/ci.yml/badge.svg)](https://github.com/Endeavoury/Ratatoskr/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Endeavoury/Ratatoskr?label=release)](https://github.com/Endeavoury/Ratatoskr/releases/latest)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Ratatoskr is a cross-language networking SDK with one portable C implementation,
many language bindings, and one native command: `ratos`. DNS is the first production
protocol. Future protocols will be added as independent modules without duplicating
their implementations in each language.

> **Milestone status:** the native DNS path, CLI, C ABI, and .NET binding are usable.
> The ABI is version 1, but the product is pre-1.0 and its API should still be treated
> as an initial compatibility contract.

## Build

Requirements: a C11 compiler and CMake 3.20 or newer.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On multi-configuration generators, add `--config Release` to build, test, and install.

```sh
cmake --install build --prefix /usr/local
```

For repeatable local builds, equivalent `dev`, `release`, and `sanitize` CMake
presets are included:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

This installs `ratos`, the shared/static library, public headers, pkg-config metadata,
and CMake target metadata. The native core has no .NET dependency.

## CLI

```sh
ratos dns example.com
ratos dns a example.com
ratos dns aaaa example.com
ratos dns mx example.com
ratos dns ptr 1.1.1.1
ratos dns ptr 2001:4860:4860::8888
ratos dns example.com --server 1.1.1.1
ratos dns example.com --timeout 3000
ratos dns example.com --json
```

Human output writes only answer data to stdout. Errors and `--verbose` diagnostics go
to stderr, so `IP="$(ratos dns a example.com | head -1)"` is safe. See
[DNS behavior](docs/dns.md) for types, JSON, timeouts, and exit codes.

## C API

```c
#include <ratatoskr/ratatoskr.h>

ratos_context *ctx = ratos_context_create();
ratos_dns_query_options options;
ratos_dns_query_options_init(&options);
options.type = RATOS_DNS_A;
options.server = "1.1.1.1";

ratos_dns_result *result = NULL;
ratos_error error = ratos_dns_query(ctx, "example.com", &options, &result);
if (error == RATOS_OK) {
    for (size_t i = 0; i < ratos_dns_result_count(result); ++i)
        puts(ratos_dns_record_text(ratos_dns_result_record(result, i)));
}
ratos_dns_result_destroy(result);
ratos_context_destroy(ctx);
```

All returned allocations stay owned by Ratatoskr. See [ABI policy](docs/abi.md).

## .NET

```csharp
using Ratatoskr;

var result = await Dns.QueryAsync("example.com", DnsRecordType.A);
foreach (var record in result.Answers)
    Console.WriteLine(record);
```

The .NET package uses source-generated native interop and `SafeHandle`; it calls
`libratatoskr` directly. The legacy `DNS.Client` namespace remains as a compatibility
adapter for existing consumers. See [migration notes](docs/migration-dotnet.md).

## Layout

```text
include/ratatoskr/       stable public C API
src/core/                protocol-independent runtime
src/protocols/<name>/    independently built protocol modules
cli/commands/            thin command adapters and registry
bindings/<language>/     language wrappers, tests, and packaging
tests/                   native unit, ABI, fixture, and integration tests
fuzz/<protocol>/         parser fuzz targets and corpora
cmake/                   reusable build policy and installation modules
packaging/               native operating-system packaging
docs/                    architecture, ABI, protocol, and binding contracts
```

Read the [documentation index](docs/README.md), [architecture](docs/architecture.md),
[repository layout](docs/repository-layout.md),
[binding roadmap](bindings/README.md), [DNS documentation](docs/dns.md), and the
[C ABI contract](docs/abi.md). Contributions should keep the central rule intact:

**One implementation. Many languages. One CLI.**

Released under the [MIT License](LICENSE).
