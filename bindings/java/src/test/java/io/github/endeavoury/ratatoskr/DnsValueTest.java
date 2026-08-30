package io.github.endeavoury.ratatoskr;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.time.Duration;
import java.util.List;
import org.junit.jupiter.api.Test;

class DnsValueTest {
    @Test
    void optionsBuilderAppliesDefaultsAndOverrides() {
        DnsQueryOptions options = DnsQueryOptions.builder()
            .server("2001:4860:4860::8888")
            .port(5353)
            .timeout(Duration.ofMillis(750))
            .recursionDesired(false)
            .build();

        assertEquals("2001:4860:4860::8888", options.server());
        assertEquals(5353, options.port());
        assertEquals(Duration.ofMillis(750), options.timeout());
        assertEquals(false, options.recursionDesired());
    }

    @Test
    void optionsRejectUnboundedOrInvalidNetworkValues() {
        assertThrows(IllegalArgumentException.class,
            () -> new DnsQueryOptions(null, 0, Duration.ofSeconds(1), true));
        assertThrows(IllegalArgumentException.class,
            () -> new DnsQueryOptions(null, 53, Duration.ZERO, true));
    }

    @Test
    void recordsDefensivelyCopyRawData() {
        byte[] bytes = {1, 2, 3};
        DnsRecord record = new DnsRecord(65_000, DnsSection.ANSWER, "example.com", 60,
            "opaque", bytes, List.of(), List.of(), List.of());
        bytes[0] = 9;
        byte[] returned = record.rawData();
        returned[1] = 9;

        assertArrayEquals(new byte[] {1, 2, 3}, record.rawData());
        assertEquals(true, record.type().isEmpty());
    }
}
