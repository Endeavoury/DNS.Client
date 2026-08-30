# Publishing Ratatoskr

Tagged `v*` releases run `package.yml`. It produces the C core for Linux, macOS, and
Windows across x64/ARM64 and merges them into the `Ratatoskr` NuGet runtime layout:

```text
runtimes/linux-x64/native/libratatoskr.so
runtimes/linux-arm64/native/libratatoskr.so
runtimes/win-x64/native/ratatoskr.dll
runtimes/win-arm64/native/ratatoskr.dll
runtimes/osx-x64/native/libratatoskr.dylib
runtimes/osx-arm64/native/libratatoskr.dylib
```

Before tagging, run native tests, sanitizers, managed interop tests, CLI smoke tests,
and inspect exported symbols. Update `CHANGELOG.md`, the C product-version macros, the
CMake project version, man page, and Arch `pkgver` together. Increment ABI version and
the shared-library major only for binary-incompatible changes.

The workflow packs both `Ratatoskr` and the dependency-based `Endeavoury.DNS.Client`
compatibility package. It uploads artifacts to GitHub. Publishing to package registries is
an explicit maintainer action until signing and registry trusted-publishing policies
are configured for the Ratatoskr repository.
