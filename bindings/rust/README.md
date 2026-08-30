# Rust binding plan

A `-sys` crate declares ABI 1 and a safe crate owns opaque handles with `Drop`. The C
implementation remains authoritative.
