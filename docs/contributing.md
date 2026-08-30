# Contributing to Ratatoskr

Build and test native changes first:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DRATOS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The equivalent preset workflow is `cmake --preset dev`, `cmake --build --preset dev`,
and `ctest --preset dev`. Keep new component sources in their local `CMakeLists.txt`;
the root build file should remain orchestration-only.

For Linux sanitizer checks add `-DRATOS_ENABLE_SANITIZERS=ON`. Clang/libFuzzer
targets use `-DRATOS_BUILD_FUZZERS=ON`.

Managed changes require .NET 8:

```sh
dotnet build bindings/dotnet/Ratatoskr.sln
LD_LIBRARY_PATH="$PWD/build" dotnet test bindings/dotnet/tests/Ratatoskr.Compatibility.Tests/Ratatoskr.Compatibility.Tests.csproj
```

Keep public native symbols prefixed `ratos_`, ownership explicit, network reads
bounds-checked, the CLI thin, and protocol behavior in C. Add malformed-input tests for
every parser change. Do not add protocol implementations to language wrappers.
