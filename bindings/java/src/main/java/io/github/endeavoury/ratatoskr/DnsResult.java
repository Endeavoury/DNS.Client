package io.github.endeavoury.ratatoskr;

import java.util.List;

/** Immutable structured DNS response copied from the native result. */
public record DnsResult(
    String queryName,
    DnsRecordType queryType,
    String server,
    int transactionId,
    int responseCode,
    String responseCodeName,
    boolean authoritative,
    boolean truncated,
    boolean recursionDesired,
    boolean recursionAvailable,
    boolean authenticData,
    boolean checkingDisabled,
    List<DnsRecord> records
) {
    /** Creates a response with an immutable record collection. */
    public DnsResult {
        records = List.copyOf(records);
    }

    /** Returns records from the answer section only. */
    public List<DnsRecord> answers() {
        return records.stream().filter(record -> record.section() == DnsSection.ANSWER).toList();
    }

    /** Returns whether the DNS response code is NOERROR. */
    public boolean successful() {
        return responseCode == 0;
    }
}
