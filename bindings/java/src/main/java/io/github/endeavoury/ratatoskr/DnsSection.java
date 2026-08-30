package io.github.endeavoury.ratatoskr;

/** Section containing a DNS resource record. */
public enum DnsSection {
    ANSWER(1), AUTHORITY(2), ADDITIONAL(3);

    private final int code;

    DnsSection(int code) {
        this.code = code;
    }

    /** Returns the native section code. */
    public int code() {
        return code;
    }

    /** Converts a native section code. */
    public static DnsSection fromCode(int code) {
        for (DnsSection section : values()) {
            if (section.code == code) return section;
        }
        throw new IllegalArgumentException("Unknown DNS section: " + code);
    }
}
