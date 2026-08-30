#!/usr/bin/env python3
"""Deterministic local DNS server exercising ratos UDP-to-TCP fallback."""

import json
import socket
import struct
import subprocess
import sys
import threading


def question_end(packet: bytes) -> int:
    position = 12
    while True:
        length = packet[position]
        position += 1
        if length == 0:
            return position + 4
        position += length


def serve(udp: socket.socket, tcp: socket.socket) -> None:
    query, peer = udp.recvfrom(512)
    end = question_end(query)
    truncated = query[:2] + b"\x83\x80\x00\x01\x00\x00\x00\x00\x00\x00" + query[12:end]
    udp.sendto(truncated, peer)
    connection, _ = tcp.accept()
    with connection:
        size = struct.unpack("!H", connection.recv(2))[0]
        request = b""
        while len(request) < size:
            request += connection.recv(size - len(request))
        end = question_end(request)
        answer = (request[:2] + b"\x81\x80\x00\x01\x00\x01\x00\x00\x00\x00"
                  + request[12:end] + b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04\xc0\x00\x02\x2a")
        connection.sendall(struct.pack("!H", len(answer)) + answer)


def main() -> int:
    if len(sys.argv) != 2:
        return 2
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp:
        tcp.bind(("127.0.0.1", 0))
        tcp.listen(1)
        port = tcp.getsockname()[1]
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.bind(("127.0.0.1", port))
            worker = threading.Thread(target=serve, args=(udp, tcp), daemon=True)
            worker.start()
            completed = subprocess.run(
                [sys.argv[1], "dns", "a", "example.com", "--server", "127.0.0.1",
                 "--port", str(port), "--timeout", "2000", "--json"],
                check=False, capture_output=True, text=True, timeout=5)
            worker.join(timeout=2)
    if completed.returncode != 0 or completed.stderr:
        print(completed.stderr, file=sys.stderr)
        return 1
    payload = json.loads(completed.stdout)
    records = payload["response"]["records"]
    return 0 if records[0]["address"] == "192.0.2.42" else 1


if __name__ == "__main__":
    raise SystemExit(main())
