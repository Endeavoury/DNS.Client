package io.github.endeavoury.ratatoskr;

/** Stable machine-readable errors from the Ratatoskr C ABI. */
public enum RatosError {
    OK(0), GENERIC(1), INVALID_ARGUMENT(2), OUT_OF_MEMORY(3), TIMEOUT(4),
    NETWORK(5), PROTOCOL(6), DNS(7), NOT_FOUND(8), UNSUPPORTED(9),
    PERMISSION_DENIED(10);

    private final int code;

    RatosError(int code) {
        this.code = code;
    }

    /** Returns the stable native error code. */
    public int code() {
        return code;
    }

    /** Converts a stable native code, using {@link #GENERIC} for an unknown future code. */
    public static RatosError fromCode(int code) {
        for (RatosError error : values()) {
            if (error.code == code) return error;
        }
        return GENERIC;
    }
}
