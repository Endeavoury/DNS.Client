# Ratatoskr for Swift

The Swift package imports the installed C headers through `CRatatoskr` and adds
Swift value types with deterministic native ownership.

```swift
let result = try Dns.query("example.com", type: .a)
```
