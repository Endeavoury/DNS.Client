# Ratatoskr for .NET

This binding calls `libratatoskr` directly through the stable C ABI. It never shells
out to `ratos` and contains no independent DNS protocol implementation.

The package targets .NET 8 and uses source-generated `LibraryImport` declarations and
`SafeHandle` ownership. `QueryAsync` currently runs the synchronous native call on a
worker thread. Cancellation is observed before and after that call; native in-flight
cancellation will be added with the future task ABI.

Release packaging places native artifacts under `runtimes/<rid>/native/` for
`linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`, `osx-x64`, and `osx-arm64`.

```text
src/Ratatoskr/                         canonical managed API
src/Ratatoskr.Compatibility/           legacy DNS.Client adapter
tests/Ratatoskr.Compatibility.Tests/   managed and native-boundary tests
samples/LegacyDnsClient/               compatibility sample
runtimes/                              NuGet native runtime staging
Ratatoskr.sln                          binding-local solution
```
