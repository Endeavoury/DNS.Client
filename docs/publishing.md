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

Python release staging produces one `py3-none-<platform>` wheel for each native target
and a native-free source distribution. The build tool verifies that every wheel holds
exactly one native library and that the requested release version matches
`bindings/python/src/ratatoskr/_version.py`. CI installs the Linux wheel in a clean
virtual environment and runs the DNS fixture tests without a system-library override.
The PyPI distribution is `ratatoskr-sdk` while its import is `ratatoskr`; the shorter
distribution name is owned by an unrelated project.

Before tagging, run native tests, sanitizers, binding interop tests, CLI smoke tests,
and inspect exported symbols. Update `CHANGELOG.md`, the C product-version macros, the
CMake project version, Python `_version.py`, man page, and Arch `pkgver` together.
Increment ABI version and the shared-library major only for binary-incompatible changes.

The workflow packs `Ratatoskr`, the dependency-based `Endeavoury.DNS.Client`
compatibility package, `io.github.endeavoury:ratatoskr`, and `ratatoskr-sdk`. It uploads
all artifacts to GitHub. Publishing to NuGet or Maven Central remains an explicit
maintainer action until signing and registry policies are configured for the repository.

For PyPI, configure a Trusted Publisher for the repository, `package.yml`, the `pypi`
environment, and distribution `ratatoskr-sdk`. A maintainer can then manually dispatch
the release workflow with `publish_pypi` enabled. The publish job uses OpenID Connect;
no long-lived PyPI API token is stored in GitHub.

For Maven Central, first verify the `io.github.endeavoury` namespace and configure the
`central` token plus an OpenPGP key. Then run:

```sh
mvn -f bindings/java/pom.xml deploy -Prelease -Drevision=0.1.0
```
