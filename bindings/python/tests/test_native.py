from __future__ import annotations

import asyncio
import os
import socket
import struct
import threading
import unittest
from typing import Self

import ratatoskr

NATIVE_TESTS_ENABLED = bool(
    os.environ.get("RATATOSKR_LIBRARY") or os.environ.get("RATATOSKR_TEST_NATIVE")
)


def _question_end(packet: bytes) -> int:
    position = 12
    while position < len(packet):
        length = packet[position]
        position += 1
        if length == 0:
            if position + 4 > len(packet):
                raise ValueError("truncated DNS question")
            return position + 4
        position += length
    raise ValueError("unterminated DNS name")


def _response(query: bytes, flags: int, answers: int) -> bytes:
    end = _question_end(query)
    transaction_id = struct.unpack_from("!H", query)[0]
    header = struct.pack("!HHHHHH", transaction_id, flags, 1, answers, 0, 0)
    return header + query[12:end]


class TruncatedFixture:
    def __enter__(self) -> Self:
        self.tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp.bind(("127.0.0.1", 0))
        self.tcp.listen(1)
        self.port = self.tcp.getsockname()[1]
        self.udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp.bind(("127.0.0.1", self.port))
        self.error: BaseException | None = None
        self.thread = threading.Thread(target=self._serve, daemon=True)
        self.thread.start()
        return self

    def _serve(self) -> None:
        try:
            query, address = self.udp.recvfrom(512)
            self.udp.sendto(_response(query, 0x8380, 0), address)
            connection, _ = self.tcp.accept()
            with connection:
                length_bytes = connection.recv(2)
                if len(length_bytes) != 2:
                    raise RuntimeError("TCP fixture did not receive a length prefix")
                length = struct.unpack("!H", length_bytes)[0]
                tcp_query = b""
                while len(tcp_query) < length:
                    chunk = connection.recv(length - len(tcp_query))
                    if not chunk:
                        raise RuntimeError("TCP fixture request ended early")
                    tcp_query += chunk
                answer = _response(tcp_query, 0x8180, 1) + bytes(
                    [
                        0xC0,
                        0x0C,
                        0x00,
                        0x01,
                        0x00,
                        0x01,
                        0x00,
                        0x00,
                        0x00,
                        0x3C,
                        0x00,
                        0x04,
                        0xC0,
                        0x00,
                        0x02,
                        0x2A,
                    ]
                )
                connection.sendall(struct.pack("!H", len(answer)) + answer)
        except (OSError, RuntimeError, ValueError, struct.error) as exception:
            self.error = exception

    def __exit__(
        self,
        exception_type: type[BaseException] | None,
        exception: BaseException | None,
        traceback: object,
    ) -> None:
        self.thread.join(timeout=3)
        self.udp.close()
        self.tcp.close()
        if exception is None and self.error is not None:
            raise self.error
        if exception is None and self.thread.is_alive():
            raise RuntimeError("DNS fixture did not finish")


@unittest.skipUnless(NATIVE_TESTS_ENABLED, "set RATATOSKR_LIBRARY for native tests")
class NativeIntegrationTests(unittest.TestCase):
    def test_native_versions(self) -> None:
        self.assertEqual(ratatoskr.abi_version(), 1)
        self.assertEqual(ratatoskr.native_version(), (0, 1, 0))

    def test_udp_truncation_falls_back_to_tcp_and_copies_result(self) -> None:
        with TruncatedFixture() as fixture:
            result = ratatoskr.dns.query(
                "example.com",
                "A",
                server="127.0.0.1",
                port=fixture.port,
                timeout_ms=2_000,
            )
        self.assertTrue(result.successful)
        self.assertFalse(result.truncated)
        self.assertEqual(result.response_code_name, "NOERROR")
        self.assertEqual(result.answers[0].type, ratatoskr.DnsRecordType.A)
        self.assertEqual(result.answers[0].text, "192.0.2.42")
        self.assertEqual(result.answers[0].raw_data, bytes([192, 0, 2, 42]))

    def test_asyncio_api_uses_same_native_path(self) -> None:
        async def resolve() -> ratatoskr.DnsResult:
            with TruncatedFixture() as fixture:
                return await ratatoskr.dns.query_async(
                    "example.com",
                    ratatoskr.DnsRecordType.A,
                    server="127.0.0.1",
                    port=fixture.port,
                    timeout_ms=2_000,
                )

        self.assertEqual(asyncio.run(resolve()).answers[0].text, "192.0.2.42")

    def test_timeout_is_a_typed_error(self) -> None:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as silent:
            silent.bind(("127.0.0.1", 0))
            with self.assertRaises(ratatoskr.RatatoskrError) as raised:
                ratatoskr.dns.query(
                    "example.com",
                    server="127.0.0.1",
                    port=silent.getsockname()[1],
                    timeout_ms=100,
                )
        self.assertIs(raised.exception.code, ratatoskr.ErrorCode.TIMEOUT)


if __name__ == "__main__":
    unittest.main()
