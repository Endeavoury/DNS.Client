from __future__ import annotations

import unittest

import ratatoskr
from ratatoskr import DnsQueryOptions, DnsRecord, DnsRecordType, DnsSection


class ModelTests(unittest.TestCase):
    def test_record_type_accepts_names_enums_and_codes(self) -> None:
        self.assertIs(DnsRecordType.coerce("aaaa"), DnsRecordType.AAAA)
        self.assertIs(DnsRecordType.coerce(DnsRecordType.MX), DnsRecordType.MX)
        self.assertIs(DnsRecordType.coerce(257), DnsRecordType.CAA)
        with self.assertRaises(ValueError):
            DnsRecordType.coerce("HTTPS")
        with self.assertRaises(TypeError):
            DnsRecordType.coerce(True)

    def test_options_validate_network_values(self) -> None:
        self.assertEqual(DnsQueryOptions().port, 53)
        with self.assertRaises(ValueError):
            DnsQueryOptions(port=0)
        with self.assertRaises(ValueError):
            DnsQueryOptions(timeout_ms=0)
        with self.assertRaises(ValueError):
            DnsQueryOptions(server=" ")
        with self.assertRaises(ValueError):
            DnsQueryOptions(server="127.0.0.1\0ignored")
        with self.assertRaises(TypeError):
            DnsQueryOptions(port=True)
        with self.assertRaises(TypeError):
            DnsQueryOptions(timeout_ms=1.5)  # type: ignore[arg-type]

    def test_unknown_record_type_preserves_immutable_raw_data(self) -> None:
        source = bytearray(b"opaque")
        record = DnsRecord(65_000, DnsSection.ANSWER, "example.com", 60, "", source)
        source[0] = ord("X")
        self.assertIsNone(record.type)
        self.assertEqual(record.raw_data, b"opaque")
        with self.assertRaises(AttributeError):
            record.ttl = 10  # type: ignore[misc]

    def test_query_rejects_invalid_names_before_loading_native_code(self) -> None:
        with self.assertRaises(ValueError):
            ratatoskr.dns.query("example.com\0ignored")
        with self.assertRaises(TypeError):
            ratatoskr.dns.query(None)  # type: ignore[arg-type]


if __name__ == "__main__":
    unittest.main()
