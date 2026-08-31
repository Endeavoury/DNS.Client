# Ratatoskr for Ruby

The `ratatoskr-sdk` gem uses Ruby's standard `Fiddle` library to call the native
core. Set `RATATOSKR_LIBRARY` when it is not in the system loader path.

```ruby
result = Ratatoskr.query("example.com", type: :a)
```
