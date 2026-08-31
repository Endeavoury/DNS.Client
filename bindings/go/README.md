# Ratatoskr for Go

This module uses cgo and the installed `ratatoskr.pc` file to call the canonical
C core. Install `libratatoskr` and ensure `PKG_CONFIG_PATH` can find it.

```go
result, err := ratatoskr.Query("example.com", ratatoskr.A, ratatoskr.QueryOptions{})
```
