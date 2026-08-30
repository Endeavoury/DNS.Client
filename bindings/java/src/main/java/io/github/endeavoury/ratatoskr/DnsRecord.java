package io.github.endeavoury.ratatoskr;

import java.util.List;
import java.util.Optional;

/** Immutable DNS record copied out of native ownership. */
public record DnsRecord(
    int typeCode,
    DnsSection section,
    String name,
    long timeToLive,
    String text,
    byte[] rawData,
    List<Integer> unsigned16Fields,
    List<Long> unsigned32Fields,
    List<String> stringFields
) {
    /** Creates an immutable record and defensively copies all collections and bytes. */
    public DnsRecord {
        if (typeCode < 0 || typeCode > 65_535) throw new IllegalArgumentException("invalid type code");
        if (timeToLive < 0 || timeToLive > 0xffff_ffffL) throw new IllegalArgumentException("invalid TTL");
        rawData = rawData.clone();
        unsigned16Fields = List.copyOf(unsigned16Fields);
        unsigned32Fields = List.copyOf(unsigned32Fields);
        stringFields = List.copyOf(stringFields);
    }

    /** Returns a defensive copy of the original RDATA bytes. */
    @Override
    public byte[] rawData() {
        return rawData.clone();
    }

    /** Returns a known record type, or empty when raw unknown RDATA was preserved. */
    public Optional<DnsRecordType> type() {
        return DnsRecordType.fromCode(typeCode);
    }

    @Override
    public String toString() {
        return text;
    }
}
