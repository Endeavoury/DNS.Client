"""Ownership-safe translation between Python values and the C ABI."""

from __future__ import annotations

import ctypes

from ._loader import NativeDnsQueryOptions, load_library
from .errors import ErrorCode, RatatoskrError
from .models import DnsQueryOptions, DnsRecord, DnsRecordType, DnsResult, DnsSection

_MAX_RECORDS = 4_096
_MAX_RDATA = 65_535
_MAX_STRING_FIELDS = 65_535
_MAX_NUMERIC_FIELDS = 64


def _string(value: bytes | None) -> str:
    return "" if value is None else value.decode("utf-8", errors="replace")


def _numeric_fields(library: ctypes.CDLL, record: int, bits: int) -> tuple[int, ...]:
    value_type = ctypes.c_uint16 if bits == 16 else ctypes.c_uint32
    function = (
        library.ratos_dns_record_uint16
        if bits == 16
        else library.ratos_dns_record_uint32
    )
    values: list[int] = []
    for index in range(_MAX_NUMERIC_FIELDS):
        output = value_type()
        if not function(record, index, ctypes.byref(output)):
            return tuple(values)
        values.append(int(output.value))
    raise RuntimeError("native DNS record exceeded the numeric field safety limit")


def _copy_record(library: ctypes.CDLL, record: int) -> DnsRecord:
    raw_length = ctypes.c_size_t()
    raw_pointer = library.ratos_dns_record_raw_data(record, ctypes.byref(raw_length))
    length = int(raw_length.value)
    if length > _MAX_RDATA:
        raise RuntimeError(f"native DNS RDATA exceeds {_MAX_RDATA} bytes")
    if length and not raw_pointer:
        raise RuntimeError(
            "native DNS record returned null RDATA with a nonzero length"
        )
    raw_data = ctypes.string_at(raw_pointer, length) if length else b""

    string_count = int(library.ratos_dns_record_string_count(record))
    if string_count > _MAX_STRING_FIELDS:
        raise RuntimeError("native DNS record exceeded the string field safety limit")
    strings = tuple(
        _string(library.ratos_dns_record_string(record, index))
        for index in range(string_count)
    )
    try:
        section = DnsSection(int(library.ratos_dns_record_section(record)))
    except ValueError as exception:
        raise RuntimeError(
            "native DNS record returned an unknown section"
        ) from exception

    return DnsRecord(
        type_code=int(library.ratos_dns_record_type_code(record)),
        section=section,
        name=_string(library.ratos_dns_record_name(record)),
        ttl=int(library.ratos_dns_record_ttl(record)),
        text=_string(library.ratos_dns_record_text(record)),
        raw_data=raw_data,
        uint16_fields=_numeric_fields(library, record, 16),
        uint32_fields=_numeric_fields(library, record, 32),
        string_fields=strings,
    )


def _copy_result(library: ctypes.CDLL, result: int) -> DnsResult:
    count = int(library.ratos_dns_result_count(result))
    if count > _MAX_RECORDS:
        raise RuntimeError(f"native DNS result exceeds {_MAX_RECORDS} records")
    records: list[DnsRecord] = []
    for index in range(count):
        record = library.ratos_dns_result_record(result, index)
        if not record:
            raise RuntimeError(
                f"native DNS result contains a null record at index {index}"
            )
        records.append(_copy_record(library, record))

    response_code = int(library.ratos_dns_result_rcode(result))
    try:
        query_type = DnsRecordType(int(library.ratos_dns_result_query_type(result)))
    except ValueError as exception:
        raise RuntimeError(
            "native DNS result returned an unknown query type"
        ) from exception
    return DnsResult(
        query_name=_string(library.ratos_dns_result_query_name(result)),
        query_type=query_type,
        server=_string(library.ratos_dns_result_server(result)),
        transaction_id=int(library.ratos_dns_result_transaction_id(result)),
        response_code=response_code,
        response_code_name=_string(library.ratos_dns_rcode_string(response_code)),
        authoritative=bool(library.ratos_dns_result_authoritative(result)),
        truncated=bool(library.ratos_dns_result_truncated(result)),
        recursion_desired=bool(library.ratos_dns_result_recursion_desired(result)),
        recursion_available=bool(library.ratos_dns_result_recursion_available(result)),
        authentic_data=bool(library.ratos_dns_result_authentic_data(result)),
        checking_disabled=bool(library.ratos_dns_result_checking_disabled(result)),
        records=tuple(records),
    )


def query_native(
    name: str, record_type: DnsRecordType, options: DnsQueryOptions
) -> DnsResult:
    """Call the C core and release all native ownership on every path."""

    if not isinstance(name, str):
        raise TypeError("name must be a string")
    if not name.strip() or "\0" in name:
        raise ValueError("name must not be empty or contain NUL")
    try:
        encoded_name = name.encode("idna")
        encoded_server = (
            options.server.encode("ascii") if options.server is not None else None
        )
    except UnicodeError as exception:
        raise ValueError(
            "name or server could not be encoded for the native API"
        ) from exception

    library = load_library()
    context = library.ratos_context_create()
    if not context:
        raise MemoryError("could not allocate a Ratatoskr context")
    result = ctypes.c_void_p()
    try:
        native_options = NativeDnsQueryOptions()
        library.ratos_dns_query_options_init(ctypes.byref(native_options))
        native_options.server = encoded_server
        native_options.port = options.port
        native_options.type = int(record_type)
        native_options.timeout_ms = options.timeout_ms
        native_options.recursion_desired = int(options.recursion_desired)

        error_value = int(
            library.ratos_dns_query(
                context,
                encoded_name,
                ctypes.byref(native_options),
                ctypes.byref(result),
            )
        )
        if error_value != 0:
            try:
                code = ErrorCode(error_value)
            except ValueError:
                code = ErrorCode.GENERIC
            summary = _string(library.ratos_error_string(error_value))
            detail = _string(library.ratos_context_error(context))
            message = (
                f"{summary}: {detail}"
                if detail and detail != summary
                else (detail or summary)
            )
            raise RatatoskrError(code, message or f"native error {error_value}")
        if not result.value:
            raise RuntimeError("native DNS query succeeded without a result")
        return _copy_result(library, result.value)
    finally:
        if result.value:
            library.ratos_dns_result_destroy(result)
        library.ratos_context_destroy(context)


def abi_version() -> int:
    return int(load_library().ratos_abi_version())


def native_version() -> tuple[int, int, int]:
    library = load_library()
    return (
        int(library.ratos_version_major()),
        int(library.ratos_version_minor()),
        int(library.ratos_version_patch()),
    )
