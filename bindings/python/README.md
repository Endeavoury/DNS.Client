# Ratatoskr for Python

Python bindings for the Ratatoskr networking SDK. The package is a thin,
dependency-free `ctypes` layer over the canonical native C core; it does not
contain a second DNS implementation and never invokes the `ratos` CLI.

```sh
pip install ratatoskr-sdk
```

The distribution is named `ratatoskr-sdk` because the unrelated `ratatoskr`
project name is already registered on PyPI. The import remains simply
`ratatoskr`.

```python
import ratatoskr

result = ratatoskr.dns.query("example.com", record_type="A")
for record in result.answers:
    print(record.text)
```

Async applications can move the synchronous native v1 API to an executor:

```python
result = await ratatoskr.dns.query_async(
    "example.com",
    record_type=ratatoskr.DnsRecordType.AAAA,
    server="1.1.1.1",
    timeout_ms=3_000,
)
```

`DnsClient` is useful when several calls share resolver settings:

```python
from ratatoskr import DnsClient, DnsQueryOptions, DnsRecordType

client = DnsClient(DnsQueryOptions(server="1.1.1.1", timeout_ms=2_000))
result = client.query("example.com", DnsRecordType.MX)
```

All returned objects are immutable Python values copied before native ownership
is released. Unknown resource-record types retain their numeric `type_code` and
raw RDATA bytes.

## Native library discovery

The loader checks these locations in order:

1. `RATATOSKR_LIBRARY`, containing the exact library file path.
2. The platform library bundled in the installed wheel.
3. A system-installed `ratatoskr` library discoverable by the dynamic loader.

Official wheels target Linux, macOS, and Windows on x86-64 and ARM64. A source
distribution expects Ratatoskr to be installed separately or supplied through
`RATATOSKR_LIBRARY`.

## Develop and test

From the repository root, build the native core and run the standard-library
test suite:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRATOS_BUILD_CLI=OFF
cmake --build build --parallel
RATATOSKR_LIBRARY="$(pwd)/build/libratatoskr.so" \
  PYTHONPATH=bindings/python/src \
  python -m unittest discover -s bindings/python/tests -v
```

Build a native-free source distribution with:

```sh
python -m build --sdist bindings/python
```

Release wheels must be built with `tools/build_release_wheels.py` after all six
native artifacts have been staged. The tool rejects missing libraries, mixed
platform contents, nonempty output directories, and version mismatches.

Ratatoskr for Python requires Python 3.11 or newer and native ABI version 1.
