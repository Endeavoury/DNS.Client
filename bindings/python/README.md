# Python binding plan

The future `ratatoskr` wheel will use cffi in ABI mode, bundle the native library, and
expose `ratatoskr.dns.query`. No DNS implementation lives here.
