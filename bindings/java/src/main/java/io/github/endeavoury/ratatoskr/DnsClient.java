package io.github.endeavoury.ratatoskr;

import io.github.endeavoury.ratatoskr.internal.NativeDns;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executor;
import java.util.concurrent.ForkJoinPool;

/** Thread-safe DNS client backed entirely by the Ratatoskr native C core. */
public final class DnsClient {
    private final DnsQueryOptions options;
    private final Executor executor;

    /** Creates a client using OS resolver defaults. */
    public DnsClient() {
        this(DnsQueryOptions.defaults());
    }

    /** Creates a client using immutable query settings. */
    public DnsClient(DnsQueryOptions options) {
        this(options, ForkJoinPool.commonPool());
    }

    /** Creates a client whose asynchronous operations use the supplied executor. */
    public DnsClient(DnsQueryOptions options, Executor executor) {
        this.options = Objects.requireNonNull(options, "options");
        this.executor = Objects.requireNonNull(executor, "executor");
    }

    /** Performs a synchronous DNS query through the native core. */
    public DnsResult query(String name, DnsRecordType type) {
        if (name == null || name.isBlank()) throw new IllegalArgumentException("name must be non-blank");
        Objects.requireNonNull(type, "type");
        return NativeDns.query(name, type, options);
    }

    /** Performs a synchronous A query. */
    public DnsResult query(String name) {
        return query(name, DnsRecordType.A);
    }

    /** Schedules a DNS query on this client's executor. */
    public CompletableFuture<DnsResult> queryAsync(String name, DnsRecordType type) {
        return CompletableFuture.supplyAsync(() -> query(name, type), executor);
    }

    /** Schedules an A query on this client's executor. */
    public CompletableFuture<DnsResult> queryAsync(String name) {
        return queryAsync(name, DnsRecordType.A);
    }

    /** Returns this client's immutable settings. */
    public DnsQueryOptions options() {
        return options;
    }
}
