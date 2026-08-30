package io.github.endeavoury.ratatoskr;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;

class NativeDnsIntegrationTest {
    @Test
    void javaCallsNativeCoreAndFollowsUdpTruncationWithTcp() throws Exception {
        assumeTrue(System.getProperty("ratatoskr.library.path") != null,
            "Pass -Dratatoskr.library.path to run native integration coverage");

        InetAddress loopback = InetAddress.getByName("127.0.0.1");
        try (ServerSocket tcp = new ServerSocket(0, 1, loopback);
             DatagramSocket udp = new DatagramSocket(tcp.getLocalPort(), loopback)) {
            CompletableFuture<Void> fixture = CompletableFuture.runAsync(() -> serve(udp, tcp));
            DnsQueryOptions options = DnsQueryOptions.builder()
                .server("127.0.0.1")
                .port(tcp.getLocalPort())
                .timeout(Duration.ofSeconds(2))
                .build();

            DnsResult result = new DnsClient(options).query("example.com", DnsRecordType.A);
            fixture.get(3, TimeUnit.SECONDS);

            assertEquals(1, Ratatoskr.abiVersion());
            assertEquals("0.1.0", Ratatoskr.version());
            assertTrue(result.successful());
            assertFalse(result.truncated());
            assertEquals("192.0.2.42", result.answers().getFirst().text());
            assertEquals(DnsRecordType.A, result.answers().getFirst().type().orElseThrow());
        }
    }

    @Test
    void nativeTimeoutBecomesTypedJavaException() throws Exception {
        assumeTrue(System.getProperty("ratatoskr.library.path") != null,
            "Pass -Dratatoskr.library.path to run native integration coverage");

        InetAddress loopback = InetAddress.getByName("127.0.0.1");
        try (DatagramSocket silentServer = new DatagramSocket(0, loopback)) {
            DnsQueryOptions options = DnsQueryOptions.builder()
                .server("127.0.0.1")
                .port(silentServer.getLocalPort())
                .timeout(Duration.ofMillis(100))
                .build();

            RatatoskrException exception = assertThrows(RatatoskrException.class,
                () -> new DnsClient(options).query("example.com", DnsRecordType.A));
            assertEquals(RatosError.TIMEOUT, exception.error());
        }
    }

    private static void serve(DatagramSocket udp, ServerSocket tcp) {
        try {
            byte[] buffer = new byte[512];
            DatagramPacket request = new DatagramPacket(buffer, buffer.length);
            udp.receive(request);
            byte[] query = java.util.Arrays.copyOf(request.getData(), request.getLength());
            int questionEnd = questionEnd(query);
            byte[] truncated = responsePrefix(query, (byte) 0x83, (byte) 0x80, 0);
            truncated = append(truncated, query, 12, questionEnd - 12);
            udp.send(new DatagramPacket(truncated, truncated.length, request.getSocketAddress()));

            try (Socket connection = tcp.accept();
                 DataInputStream input = new DataInputStream(connection.getInputStream());
                 DataOutputStream output = new DataOutputStream(connection.getOutputStream())) {
                byte[] tcpQuery = input.readNBytes(input.readUnsignedShort());
                questionEnd = questionEnd(tcpQuery);
                byte[] answer = responsePrefix(tcpQuery, (byte) 0x81, (byte) 0x80, 1);
                answer = append(answer, tcpQuery, 12, questionEnd - 12);
                answer = append(answer, new byte[] {
                    (byte) 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
                    0x00, 0x00, 0x00, 0x3c, 0x00, 0x04,
                    (byte) 0xc0, 0x00, 0x02, 0x2a
                }, 0, 16);
                output.writeShort(answer.length);
                output.write(answer);
            }
        } catch (IOException exception) {
            throw new IllegalStateException("DNS fixture failed", exception);
        }
    }

    private static byte[] responsePrefix(byte[] query, byte flags1, byte flags2, int answers) {
        return new byte[] {
            query[0], query[1], flags1, flags2,
            0x00, 0x01, (byte) (answers >>> 8), (byte) answers,
            0x00, 0x00, 0x00, 0x00
        };
    }

    private static int questionEnd(byte[] packet) {
        int position = 12;
        while (position < packet.length) {
            int length = Byte.toUnsignedInt(packet[position++]);
            if (length == 0) {
                if (position + 4 > packet.length) throw new IllegalArgumentException("truncated question");
                return position + 4;
            }
            position += length;
        }
        throw new IllegalArgumentException("unterminated DNS name");
    }

    private static byte[] append(byte[] first, byte[] second, int offset, int length) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream(first.length + length);
        output.write(first);
        output.write(second, offset, length);
        return output.toByteArray();
    }
}
