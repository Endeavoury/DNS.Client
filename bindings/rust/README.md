# Ratatoskr for Rust

The `ratatoskr-sdk` crate exposes the library name `ratatoskr` and calls the
canonical native C ABI. Install `libratatoskr` first, or set
`RATATOSKR_LIB_DIR` while building.

```rust
let result = ratatoskr::query("example.com", ratatoskr::RecordType::A,
    &ratatoskr::QueryOptions::default())?;
```
