# Language bindings and package roadmap

Every binding consumes the same `ratos_*` C ABI. A binding may provide idiomatic
types, async adaptation, and language-native error handling, but it must not contain
an independent DNS implementation or parse CLI output.

| Target | Distribution | Repository location | Status |
|---|---|---|---|
| C | Conan | `bindings/c/` | implemented canonical package |
| C++ | Conan | `bindings/cpp/` | implemented value/RAII wrapper |
| C# / .NET | NuGet | `bindings/dotnet/` | implemented foundation |
| JavaScript / TypeScript | npm | `bindings/node/` | implemented Koffi binding and declarations |
| Python | PyPI (`ratatoskr-sdk`) | `bindings/python/` | implemented ctypes binding |
| Java | Maven Central | `bindings/java/` | implemented Java 22+ FFM binding |
| Kotlin | Maven Central | `bindings/kotlin/` | implemented facade over Java FFM binding |
| Rust | crates.io (`ratatoskr-sdk`) | `bindings/rust/` | implemented safe FFI binding |
| PHP | Packagist | `bindings/php/` | implemented PHP FFI binding |
| Go | Go Modules / Git | `bindings/go/` | implemented cgo binding |
| Ruby | RubyGems (`ratatoskr-sdk`) | `bindings/ruby/` | implemented Fiddle binding |
| Dart | pub.dev | `bindings/dart/` | implemented Dart FFI binding |
| Swift | Swift Package Manager / Git | `bindings/swift/` | implemented Swift/C module binding |
| Lua | LuaRocks | `bindings/lua/` | implemented Lua C module |

Every package calls `libratatoskr` directly and is gated by the bindings workflow.
Repository- and tag-based ecosystems (Go and Swift) consume release tags; Packagist
tracks the Composer metadata in those tags. Registry credentials and trusted
publishing identities remain release-environment configuration, never repository
secrets embedded in package sources.
