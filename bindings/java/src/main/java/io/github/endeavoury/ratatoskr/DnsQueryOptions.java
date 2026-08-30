package io.github.endeavoury.ratatoskr;

import java.time.Duration;
import java.util.Objects;

/** Immutable resolver and transport settings for a DNS query. */
public record DnsQueryOptions(String server, int port, Duration timeout, boolean recursionDesired) {
    /** Creates validated query settings. A {@code null} server selects the OS resolver. */
    public DnsQueryOptions {
        if (server != null && server.isBlank()) {
            throw new IllegalArgumentException("server must be null or non-blank");
        }
        if (port < 1 || port > 65_535) {
            throw new IllegalArgumentException("port must be between 1 and 65535");
        }
        Objects.requireNonNull(timeout, "timeout");
        long milliseconds = timeout.toMillis();
        if (milliseconds < 1 || milliseconds > 0xffff_ffffL) {
            throw new IllegalArgumentException("timeout must be between 1 ms and 4294967295 ms");
        }
    }

    /** Returns default settings: OS resolver, port 53, five seconds, recursion enabled. */
    public static DnsQueryOptions defaults() {
        return new DnsQueryOptions(null, 53, Duration.ofSeconds(5), true);
    }

    /** Returns a builder initialized with defaults. */
    public static Builder builder() {
        return new Builder();
    }

    /** Builder for immutable DNS query settings. */
    public static final class Builder {
        private String server;
        private int port = 53;
        private Duration timeout = Duration.ofSeconds(5);
        private boolean recursionDesired = true;

        private Builder() {}

        /** Uses a particular IPv4 or IPv6 resolver address. */
        public Builder server(String server) {
            this.server = server;
            return this;
        }

        /** Uses a resolver port. */
        public Builder port(int port) {
            this.port = port;
            return this;
        }

        /** Uses a bounded network timeout. */
        public Builder timeout(Duration timeout) {
            this.timeout = timeout;
            return this;
        }

        /** Controls the DNS recursion-desired flag. */
        public Builder recursionDesired(boolean recursionDesired) {
            this.recursionDesired = recursionDesired;
            return this;
        }

        /** Builds validated settings. */
        public DnsQueryOptions build() {
            return new DnsQueryOptions(server, port, timeout, recursionDesired);
        }
    }
}
