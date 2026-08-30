# Ratatoskr for Java

The Java SDK is a thin Java 22+ wrapper over the canonical Ratatoskr C ABI. It uses
the finalized Foreign Function & Memory API, copies native results into immutable Java
values, and always releases native contexts and results. It does not implement DNS in
Java and never invokes the `ratos` CLI.

## Maven dependency

Once the first release is published to Maven Central:

```xml
<dependency>
  <groupId>io.github.endeavoury</groupId>
  <artifactId>ratatoskr</artifactId>
  <version>0.1.0</version>
</dependency>
```

Java's FFM API requires native access to be explicitly enabled:

```sh
java --enable-native-access=ALL-UNNAMED -jar application.jar
```

Applications using Ratatoskr as the automatic module
`io.github.endeavoury.ratatoskr` may enable that module name instead.

## Usage

```java
import io.github.endeavoury.ratatoskr.DnsRecordType;
import io.github.endeavoury.ratatoskr.DnsResult;
import io.github.endeavoury.ratatoskr.Ratatoskr;

DnsResult result = Ratatoskr.dns().query("example.com", DnsRecordType.A);
result.answers().forEach(record -> System.out.println(record.text()));
```

Resolver and timeout configuration is immutable:

```java
var options = DnsQueryOptions.builder()
    .server("1.1.1.1")
    .timeout(Duration.ofSeconds(3))
    .build();

var result = new DnsClient(options).query("example.com", DnsRecordType.MX);
```

`queryAsync` adapts the synchronous native v1 API to `CompletableFuture`. Cancellation
cannot interrupt an in-flight native query yet; the configured native timeout remains
authoritative.

## Native library loading

Published JARs bundle native libraries for Linux, macOS, and Windows on x86-64 and
ARM64. The loader first honors `-Dratatoskr.library.path=/absolute/path/to/library`,
then extracts the version-matched bundled resource to a temporary file, and finally
checks the normal system library path for development builds. Only 64-bit JVMs are
currently supported.

## Build and test

Build `libratatoskr` first, then run Maven with the explicit native path:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRATOS_BUILD_TESTS=ON
cmake --build build --parallel
mvn -f bindings/java/pom.xml verify \
  -Dratatoskr.library.path="$PWD/build/libratatoskr.so"
```

Use `libratatoskr.dylib` on macOS or `build/Release/ratatoskr.dll` on Windows. Without
the property, value-model tests run and the native integration test is skipped.

## Maven Central

The POM includes required project metadata and attaches source and Javadoc JARs. The
`release` profile signs artifacts and uses the Central Publisher Portal Maven plugin:

```sh
mvn -f bindings/java/pom.xml deploy -Prelease
```

Before publishing, maintainers must verify the `io.github.endeavoury` namespace,
configure the `central` token in Maven `settings.xml`, and provide a published OpenPGP
signing key. The release profile refuses to deploy unless all six supported native
libraries have been staged. Publishing is intentionally not automatic from untrusted
pull requests.
