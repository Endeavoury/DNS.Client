package io.github.endeavoury.ratatoskr;

import java.util.Arrays;
import java.util.Optional;

/** DNS resource-record types understood by the initial Ratatoskr ABI. */
public enum DnsRecordType {
    A(1), NS(2), MD(3), MF(4), CNAME(5), SOA(6), MB(7), MG(8), MR(9),
    NULL(10), WKS(11), PTR(12), HINFO(13), MINFO(14), MX(15), TXT(16),
    AAAA(28), SRV(33), NAPTR(35), CAA(257);

    private final int code;

    DnsRecordType(int code) {
        this.code = code;
    }

    /** Returns the unsigned DNS type code. */
    public int code() {
        return code;
    }

    /** Returns the known type for a numeric code, or an empty value for an unknown RR. */
    public static Optional<DnsRecordType> fromCode(int code) {
        return Arrays.stream(values()).filter(value -> value.code == code).findFirst();
    }
}
