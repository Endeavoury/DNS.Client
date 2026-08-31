# Ratatoskr for Lua

The LuaRocks package builds a small Lua C module that links directly to
`libratatoskr`. The native core and headers must be installed first.

```lua
local ratatoskr = require("ratatoskr")
local result = ratatoskr.query("example.com", { type = ratatoskr.A })
```
