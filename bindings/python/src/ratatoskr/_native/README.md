Release CI places exactly one of these files in each platform wheel:

- `linux-x86_64/libratatoskr.so`
- `linux-aarch64/libratatoskr.so`
- `macos-x86_64/libratatoskr.dylib`
- `macos-aarch64/libratatoskr.dylib`
- `windows-x86_64/ratatoskr.dll`
- `windows-aarch64/ratatoskr.dll`

Native binaries are build outputs and are intentionally ignored by Git.
