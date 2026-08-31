package = "ratatoskr"
version = "0.1.0-1"
source = { url = "git+https://github.com/Endeavoury/Ratatoskr.git", tag = "v0.1.0" }
description = {
  summary = "Lua binding for the Ratatoskr native networking core",
  homepage = "https://github.com/Endeavoury/Ratatoskr",
  license = "MIT"
}
dependencies = { "lua >= 5.3" }
build = {
  type = "builtin",
  modules = {
    ratatoskr = {
      sources = { "bindings/lua/src/ratatoskr_lua.c" },
      incdirs = { "include" },
      libraries = { "ratatoskr" }
    }
  }
}
