# Repository layout

Ratatoskr is organized around dependency direction, not around release format. The
portable C core is authoritative; every CLI command and language binding is a peer
consumer of its public ABI.

```text
include/ratatoskr/          versioned public C headers
src/core/                   protocol-independent runtime and ownership support
src/protocols/<protocol>/   one private implementation directory per protocol
cli/commands/               thin CLI adapters registered with the root command
bindings/<language>/        wrapper source, language tests, and package metadata
tests/core/                 ABI and runtime tests
tests/<protocol>/           protocol unit tests
tests/integration/          deterministic end-to-end tests
tests/fixtures/<protocol>/  non-sensitive binary test data
fuzz/<protocol>/            network-free fuzz targets and seed corpora
cmake/                      shared build policy and installation logic
packaging/                  native OS/package-manager integration
docs/                       architecture and public behavior contracts
```

## Dependency rules

```text
language binding ─┐
native CLI ───────┼──> public C ABI ──> core + protocol modules
C/C++ consumer ──┘
```

- Protocol modules never depend on the CLI or a language runtime.
- Bindings call `libratatoskr`; they never shell out to `ratos` or reimplement a protocol.
- CLI commands format structured core results but contain no transport or packet logic.
- Only `include/ratatoskr/` is public ABI. Files below `src/` are private and may change.
- Compatibility APIs stay inside their binding, clearly separated from the canonical API.
- Generated output belongs under `build/`, `artifacts/`, or language-specific `bin/` and
  `obj/` directories and must not become source layout.

## Adding a protocol

Add a public header, a private implementation directory with its own `CMakeLists.txt`,
focused native tests and fuzzing where it parses hostile input, and a thin CLI command
only when an interactive surface is useful. Register its source directory in
`src/protocols/CMakeLists.txt` and its command in `cli/commands/registry.c`. A protocol
does not require changes to existing bindings until its public C ABI is stable.

## Adding a language binding

Use `bindings/<language>/` as the package root. Keep wrapper source, binding-specific
tests, examples, generated declarations, and distribution metadata inside it. Native
runtime artifacts may be staged there for packaging, but their source remains the C
core. See [the binding roadmap](../bindings/README.md).
