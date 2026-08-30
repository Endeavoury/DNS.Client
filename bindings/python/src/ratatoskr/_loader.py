"""Native library discovery and ABI declaration."""

from __future__ import annotations

import atexit
import ctypes
import ctypes.util
import importlib.resources
import os
import platform
import tempfile
import threading
from pathlib import Path
from typing import Final

from .errors import NativeLibraryError, UnsupportedAbiError

EXPECTED_ABI: Final = 1
_library: ctypes.CDLL | None = None
_lock = threading.Lock()
_temporary_resources: list[Path] = []


class NativeDnsQueryOptions(ctypes.Structure):
    """ABI version 1 layout of ``ratos_dns_query_options``."""

    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("server", ctypes.c_char_p),
        ("port", ctypes.c_uint16),
        ("type", ctypes.c_uint16),
        ("timeout_ms", ctypes.c_uint32),
        ("recursion_desired", ctypes.c_uint8),
        ("reserved", ctypes.c_uint8 * 7),
    ]


def _cleanup_resources() -> None:
    for path in _temporary_resources:
        try:
            path.unlink()
            path.parent.rmdir()
        except OSError:
            pass


atexit.register(_cleanup_resources)


def _platform_details() -> tuple[str, str]:
    operating_system = platform.system().lower()
    machine = platform.machine().lower().replace("-", "_")
    architectures = {
        "amd64": "x86_64",
        "x64": "x86_64",
        "x86_64": "x86_64",
        "aarch64": "aarch64",
        "arm64": "aarch64",
    }
    try:
        architecture = architectures[machine]
    except KeyError as exception:
        raise NativeLibraryError(
            f"unsupported native architecture: {machine}"
        ) from exception
    systems = {"linux": "linux", "darwin": "macos", "windows": "windows"}
    try:
        system = systems[operating_system]
    except KeyError as exception:
        raise NativeLibraryError(
            f"unsupported native operating system: {operating_system}"
        ) from exception
    return f"{system}-{architecture}", {
        "linux": "libratatoskr.so",
        "macos": "libratatoskr.dylib",
        "windows": "ratatoskr.dll",
    }[system]


def _resource_path(platform_directory: str, filename: str) -> str | None:
    resource = importlib.resources.files("ratatoskr._native").joinpath(
        platform_directory, filename
    )
    if not resource.is_file():
        return None
    try:
        path = Path(os.fspath(resource))
        if path.is_file():
            return str(path)
    except TypeError:
        pass

    directory = Path(tempfile.mkdtemp(prefix="ratatoskr-"))
    extracted = directory / filename
    extracted.write_bytes(resource.read_bytes())
    _temporary_resources.append(extracted)
    return str(extracted)


def _candidate_paths() -> list[str]:
    candidates: list[str] = []
    if configured := os.environ.get("RATATOSKR_LIBRARY"):
        explicit = Path(configured).expanduser()
        if not explicit.is_file():
            raise NativeLibraryError(
                f"RATATOSKR_LIBRARY does not name a file: {explicit}"
            )
        candidates.append(str(explicit.resolve()))

    platform_directory, filename = _platform_details()
    if bundled := _resource_path(platform_directory, filename):
        candidates.append(bundled)
    if discovered := ctypes.util.find_library("ratatoskr"):
        candidates.append(discovered)
    candidates.extend([filename, "ratatoskr"])
    return list(dict.fromkeys(candidates))


def _configure(library: ctypes.CDLL) -> None:
    void_pointer = ctypes.c_void_p
    size_pointer = ctypes.POINTER(ctypes.c_size_t)

    library.ratos_abi_version.argtypes = []
    library.ratos_abi_version.restype = ctypes.c_uint32
    library.ratos_version_major.argtypes = []
    library.ratos_version_major.restype = ctypes.c_uint32
    library.ratos_version_minor.argtypes = []
    library.ratos_version_minor.restype = ctypes.c_uint32
    library.ratos_version_patch.argtypes = []
    library.ratos_version_patch.restype = ctypes.c_uint32
    library.ratos_error_string.argtypes = [ctypes.c_int]
    library.ratos_error_string.restype = ctypes.c_char_p

    library.ratos_context_create.argtypes = []
    library.ratos_context_create.restype = void_pointer
    library.ratos_context_destroy.argtypes = [void_pointer]
    library.ratos_context_destroy.restype = None
    library.ratos_context_error.argtypes = [void_pointer]
    library.ratos_context_error.restype = ctypes.c_char_p

    library.ratos_dns_query_options_init.argtypes = [
        ctypes.POINTER(NativeDnsQueryOptions)
    ]
    library.ratos_dns_query_options_init.restype = None
    library.ratos_dns_query.argtypes = [
        void_pointer,
        ctypes.c_char_p,
        ctypes.POINTER(NativeDnsQueryOptions),
        ctypes.POINTER(void_pointer),
    ]
    library.ratos_dns_query.restype = ctypes.c_int
    library.ratos_dns_result_destroy.argtypes = [void_pointer]
    library.ratos_dns_result_destroy.restype = None

    scalar_result_functions = {
        "ratos_dns_result_rcode": ctypes.c_uint8,
        "ratos_dns_result_transaction_id": ctypes.c_uint16,
        "ratos_dns_result_authoritative": ctypes.c_uint8,
        "ratos_dns_result_truncated": ctypes.c_uint8,
        "ratos_dns_result_recursion_desired": ctypes.c_uint8,
        "ratos_dns_result_recursion_available": ctypes.c_uint8,
        "ratos_dns_result_authentic_data": ctypes.c_uint8,
        "ratos_dns_result_checking_disabled": ctypes.c_uint8,
        "ratos_dns_result_query_type": ctypes.c_uint16,
        "ratos_dns_result_count": ctypes.c_size_t,
    }
    for name, result_type in scalar_result_functions.items():
        function = getattr(library, name)
        function.argtypes = [void_pointer]
        function.restype = result_type
    for name in ("ratos_dns_result_server", "ratos_dns_result_query_name"):
        function = getattr(library, name)
        function.argtypes = [void_pointer]
        function.restype = ctypes.c_char_p
    library.ratos_dns_result_record.argtypes = [void_pointer, ctypes.c_size_t]
    library.ratos_dns_result_record.restype = void_pointer

    scalar_record_functions = {
        "ratos_dns_record_type_code": ctypes.c_uint16,
        "ratos_dns_record_section": ctypes.c_uint8,
        "ratos_dns_record_ttl": ctypes.c_uint32,
        "ratos_dns_record_string_count": ctypes.c_size_t,
    }
    for name, result_type in scalar_record_functions.items():
        function = getattr(library, name)
        function.argtypes = [void_pointer]
        function.restype = result_type
    for name in ("ratos_dns_record_name", "ratos_dns_record_text"):
        function = getattr(library, name)
        function.argtypes = [void_pointer]
        function.restype = ctypes.c_char_p
    library.ratos_dns_record_raw_data.argtypes = [void_pointer, size_pointer]
    library.ratos_dns_record_raw_data.restype = ctypes.POINTER(ctypes.c_uint8)
    library.ratos_dns_record_uint16.argtypes = [
        void_pointer,
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint16),
    ]
    library.ratos_dns_record_uint16.restype = ctypes.c_int
    library.ratos_dns_record_uint32.argtypes = [
        void_pointer,
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint32),
    ]
    library.ratos_dns_record_uint32.restype = ctypes.c_int
    library.ratos_dns_record_string.argtypes = [void_pointer, ctypes.c_size_t]
    library.ratos_dns_record_string.restype = ctypes.c_char_p
    library.ratos_dns_rcode_string.argtypes = [ctypes.c_uint8]
    library.ratos_dns_rcode_string.restype = ctypes.c_char_p


def load_library() -> ctypes.CDLL:
    """Load, declare, and ABI-check the native core exactly once."""

    global _library
    if _library is not None:
        return _library
    with _lock:
        if _library is not None:
            return _library
        failures: list[str] = []
        for candidate in _candidate_paths():
            try:
                loaded = ctypes.CDLL(candidate)
                _configure(loaded)
                abi = int(loaded.ratos_abi_version())
                if abi != EXPECTED_ABI:
                    raise UnsupportedAbiError(
                        f"Ratatoskr native ABI {abi} is incompatible; expected {EXPECTED_ABI}"
                    )
                _library = loaded
                return loaded
            except UnsupportedAbiError:
                raise
            except (AttributeError, OSError) as exception:
                failures.append(f"{candidate}: {exception}")
        detail = "; ".join(failures)
        raise NativeLibraryError(
            "could not load the Ratatoskr native library; install a platform wheel, "
            "install libratatoskr system-wide, or set RATATOSKR_LIBRARY"
            + (f" ({detail})" if detail else "")
        )
