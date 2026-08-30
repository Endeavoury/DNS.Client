# Publishing Ratatoskr

Tagged `v*` releases run `package.yml`. It produces the C core for Linux, macOS, and
Windows across x64/ARM64 and merges them into both NuGet and Maven runtime layouts:

```text
runtimes/linux-x64/native/libratatoskr.so
runtimes/linux-arm64/native/libratatoskr.so
runtimes/win-x64/native/ratatoskr.dll
runtimes/win-arm64/native/ratatoskr.dll
runtimes/osx-x64/native/libratatoskr.dylib
runtimes/osx-arm64/native/libratatoskr.dylib
```

The Maven JAR uses `META-INF/native/<os>-<architecture>/` and includes the corresponding
`.so`, `.dylib`, or `.dll`. Release CI verifies the Java API against the Linux native
artifact before attaching the main, source, and Javadoc JARs.

Before tagging, run native tests, sanitizers, managed interop tests, CLI smoke tests,
and inspect exported symbols. Update `CHANGELOG.md`, the C product-version macros, the
CMake project version, man page, and Arch `pkgver` together. Increment ABI version and
the shared-library major only for binary-incompatible changes.

The workflow packs `Ratatoskr`, the dependency-based `Endeavoury.DNS.Client`
compatibility package, and `io.github.endeavoury:ratatoskr`. It uploads all artifacts to
GitHub. Publishing to NuGet or Maven Central remains an explicit maintainer action until
signing and registry trusted-publishing policies are configured for the repository.

For Maven Central, first verify the `io.github.endeavoury` namespace and configure the
`central` token plus an OpenPGP key. Then run:

```sh
mvn -f bindings/java/pom.xml deploy -Prelease -Drevision=0.1.0
```
