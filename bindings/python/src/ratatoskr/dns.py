"""Idiomatic DNS API backed by the native Ratatoskr core."""

from __future__ import annotations

import asyncio

from ._binding import query_native
from .models import DnsQueryOptions, DnsRecordType, DnsResult


class DnsClient:
    """DNS client sharing immutable resolver options across queries."""

    def __init__(self, options: DnsQueryOptions | None = None) -> None:
        if options is not None and not isinstance(options, DnsQueryOptions):
            raise TypeError("options must be DnsQueryOptions or None")
        self.options = options if options is not None else DnsQueryOptions()

    def query(
        self,
        name: str,
        record_type: DnsRecordType | str | int = DnsRecordType.A,
    ) -> DnsResult:
        return query_native(name, DnsRecordType.coerce(record_type), self.options)

    async def query_async(
        self,
        name: str,
        record_type: DnsRecordType | str | int = DnsRecordType.A,
    ) -> DnsResult:
        """Run the synchronous native v1 query without blocking the event loop."""

        return await asyncio.to_thread(self.query, name, record_type)


def query(
    name: str,
    record_type: DnsRecordType | str | int = DnsRecordType.A,
    *,
    server: str | None = None,
    port: int = 53,
    timeout_ms: int = 5_000,
    recursion_desired: bool = True,
) -> DnsResult:
    """Resolve one DNS name through the canonical Ratatoskr C implementation."""

    options = DnsQueryOptions(server, port, timeout_ms, recursion_desired)
    return DnsClient(options).query(name, record_type)


async def query_async(
    name: str,
    record_type: DnsRecordType | str | int = DnsRecordType.A,
    *,
    server: str | None = None,
    port: int = 53,
    timeout_ms: int = 5_000,
    recursion_desired: bool = True,
) -> DnsResult:
    """Asyncio adapter around :func:`query`. Cancellation cannot stop native v1."""

    return await asyncio.to_thread(
        query,
        name,
        record_type,
        server=server,
        port=port,
        timeout_ms=timeout_ms,
        recursion_desired=recursion_desired,
    )
