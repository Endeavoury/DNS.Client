# Changelog

## 0.1.0 - 2026-08-30

- Renamed the product to Ratatoskr and introduced the native `ratos` CLI.
- Added portable C11 core, ABI version 1, opaque ownership, stable errors and versions.
- Added secure DNS builder/parser, UDP, TCP fallback, timeouts, dual-stack resolvers,
  reverse IPv4/IPv6, common typed records, and unknown RDATA preservation.
- Added human and stable JSON CLI rendering.
- Added idiomatic .NET native binding and a `DNS.Client` compatibility adapter.
- Added a Java 22+ FFM binding and Maven Central-ready package with bundled-native
  loading, immutable DNS results, async adaptation, and native integration tests.
- Added the `ratatoskr-sdk` PyPI package with a dependency-free ctypes binding,
  immutable models, typed errors, asyncio adaptation, six native wheels, and
  deterministic native integration tests.
- Added native tests, fuzz targets, sanitizer option, packaging groundwork, and docs.
