"""Ratatoskr cross-language networking SDK for Python."""

from . import dns
from ._binding import abi_version, native_version
from ._version import __version__
from .dns import DnsClient
from .errors import ErrorCode, NativeLibraryError, RatatoskrError, UnsupportedAbiError
from .models import DnsQueryOptions, DnsRecord, DnsRecordType, DnsResult, DnsSection

__all__ = [
    "DnsClient",
    "DnsQueryOptions",
    "DnsRecord",
    "DnsRecordType",
    "DnsResult",
    "DnsSection",
    "ErrorCode",
    "NativeLibraryError",
    "RatatoskrError",
    "UnsupportedAbiError",
    "__version__",
    "abi_version",
    "dns",
    "native_version",
]
