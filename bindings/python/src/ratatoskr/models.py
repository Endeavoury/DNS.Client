"""Immutable public DNS request and response values."""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum


class DnsRecordType(IntEnum):
    """DNS resource-record type codes supported by the native core."""

    A = 1
    NS = 2
    MD = 3
    MF = 4
    CNAME = 5
    SOA = 6
    MB = 7
    MG = 8
    MR = 9
    NULL = 10
    WKS = 11
    PTR = 12
    HINFO = 13
    MINFO = 14
    MX = 15
    TXT = 16
    AAAA = 28
    SRV = 33
    NAPTR = 35
    CAA = 257

    @classmethod
    def coerce(cls, value: DnsRecordType | str | int) -> DnsRecordType:
        """Convert an enum, case-insensitive mnemonic, or numeric type code."""

        if isinstance(value, cls):
            return value
        if isinstance(value, bool):
            raise TypeError(f"unsupported DNS record type: {value!r}")
        if isinstance(value, str):
            try:
                return cls[value.strip().upper()]
            except KeyError as exception:
                raise ValueError(
                    f"unsupported DNS record type: {value!r}"
                ) from exception
        try:
            return cls(value)
        except (TypeError, ValueError) as exception:
            raise ValueError(f"unsupported DNS record type: {value!r}") from exception


class DnsSection(IntEnum):
    """Section containing a DNS resource record."""

    ANSWER = 1
    AUTHORITY = 2
    ADDITIONAL = 3


@dataclass(frozen=True, slots=True)
class DnsQueryOptions:
    """Resolver settings applied by :class:`DnsClient`."""

    server: str | None = None
    port: int = 53
    timeout_ms: int = 5_000
    recursion_desired: bool = True

    def __post_init__(self) -> None:
        if self.server is not None:
            if not isinstance(self.server, str):
                raise TypeError("server must be a string or None")
            if not self.server.strip() or "\0" in self.server:
                raise ValueError("server must not be empty or contain NUL")
        if isinstance(self.port, bool) or not isinstance(self.port, int):
            raise TypeError("port must be an integer")
        if not 1 <= self.port <= 65_535:
            raise ValueError("port must be between 1 and 65535")
        if isinstance(self.timeout_ms, bool) or not isinstance(self.timeout_ms, int):
            raise TypeError("timeout_ms must be an integer")
        if not 1 <= self.timeout_ms <= 0xFFFF_FFFF:
            raise ValueError("timeout_ms must be between 1 and 4294967295")
        if not isinstance(self.recursion_desired, bool):
            raise TypeError("recursion_desired must be a boolean")


@dataclass(frozen=True, slots=True)
class DnsRecord:
    """A DNS resource record copied out of native ownership."""

    type_code: int
    section: DnsSection
    name: str
    ttl: int
    text: str
    raw_data: bytes
    uint16_fields: tuple[int, ...] = ()
    uint32_fields: tuple[int, ...] = ()
    string_fields: tuple[str, ...] = ()

    def __post_init__(self) -> None:
        if not 0 <= self.type_code <= 65_535:
            raise ValueError("type_code must fit in uint16")
        if not 0 <= self.ttl <= 0xFFFF_FFFF:
            raise ValueError("ttl must fit in uint32")
        object.__setattr__(self, "raw_data", bytes(self.raw_data))
        object.__setattr__(self, "uint16_fields", tuple(self.uint16_fields))
        object.__setattr__(self, "uint32_fields", tuple(self.uint32_fields))
        object.__setattr__(self, "string_fields", tuple(self.string_fields))

    @property
    def type(self) -> DnsRecordType | None:
        """Known record type, or ``None`` for preserved unknown RDATA."""

        try:
            return DnsRecordType(self.type_code)
        except ValueError:
            return None

    def __str__(self) -> str:
        return self.text


@dataclass(frozen=True, slots=True)
class DnsResult:
    """Structured DNS response returned by the native core."""

    query_name: str
    query_type: DnsRecordType
    server: str
    transaction_id: int
    response_code: int
    response_code_name: str
    authoritative: bool
    truncated: bool
    recursion_desired: bool
    recursion_available: bool
    authentic_data: bool
    checking_disabled: bool
    records: tuple[DnsRecord, ...]

    def __post_init__(self) -> None:
        object.__setattr__(self, "records", tuple(self.records))

    @property
    def successful(self) -> bool:
        return self.response_code == 0

    @property
    def answers(self) -> tuple[DnsRecord, ...]:
        return tuple(
            record for record in self.records if record.section is DnsSection.ANSWER
        )

    @property
    def authorities(self) -> tuple[DnsRecord, ...]:
        return tuple(
            record for record in self.records if record.section is DnsSection.AUTHORITY
        )

    @property
    def additionals(self) -> tuple[DnsRecord, ...]:
        return tuple(
            record for record in self.records if record.section is DnsSection.ADDITIONAL
        )
