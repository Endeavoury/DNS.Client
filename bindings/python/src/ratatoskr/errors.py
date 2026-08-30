"""Stable native error codes and Python exceptions."""

from __future__ import annotations

from enum import IntEnum


class ErrorCode(IntEnum):
    """Machine-readable errors defined by the Ratatoskr C ABI."""

    OK = 0
    GENERIC = 1
    INVALID_ARGUMENT = 2
    OUT_OF_MEMORY = 3
    TIMEOUT = 4
    NETWORK = 5
    PROTOCOL = 6
    DNS = 7
    NOT_FOUND = 8
    UNSUPPORTED = 9
    PERMISSION_DENIED = 10


class RatatoskrError(RuntimeError):
    """An operation rejected by the native Ratatoskr core."""

    def __init__(self, code: ErrorCode, message: str) -> None:
        super().__init__(message)
        self.code = code


class NativeLibraryError(RuntimeError):
    """The platform's Ratatoskr native library could not be loaded."""


class UnsupportedAbiError(NativeLibraryError):
    """The loaded native library does not implement the required ABI."""
