# Language bindings and package roadmap

Every binding consumes the same `ratos_*` C ABI. A binding may provide idiomatic
types, async adaptation, and language-native error handling, but it must not contain
an independent DNS implementation or parse CLI output.

| Target | Distribution | Repository location | Status |
|---|---|---|---|
| C | Conan | `bindings/c/` | queued; canonical headers already installable |
| C++ | Conan | `bindings/cpp/` | queued |
| C# / .NET | NuGet | `bindings/dotnet/` | implemented foundation |
| JavaScript / TypeScript | npm | `bindings/node/` | design placeholder |
| Python | PyPI | `bindings/python/` | design placeholder |
| Java | Maven Central | `bindings/java/` | implemented Java 22+ FFM binding |
| Kotlin | Maven Central | `bindings/kotlin/` | queued; share JVM native layer with Java |
| Rust | crates.io | `bindings/rust/` | design placeholder |
| PHP | Packagist | `bindings/php/` | queued |
| Go | Go Modules / Git | `bindings/go/` | design placeholder |
| Ruby | RubyGems | `bindings/ruby/` | queued |
| Dart | pub.dev | `bindings/dart/` | queued |
| Swift | Swift Package Manager / Git | `bindings/swift/` | design placeholder |
| Lua | LuaRocks | `bindings/lua/` | queued |

Package publishing is deliberately staged after ABI and platform artifact validation.
Empty public packages are not published. Each package must prove an end-to-end call
from its public API through `libratatoskr` before release.
