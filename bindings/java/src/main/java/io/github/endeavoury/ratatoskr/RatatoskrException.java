package io.github.endeavoury.ratatoskr;

/** Failure returned by the native Ratatoskr core. */
public final class RatatoskrException extends RuntimeException {
    private static final long serialVersionUID = 1L;
    private final RatosError error;

    /** Creates a native-operation exception. */
    public RatatoskrException(RatosError error, String message) {
        super(message);
        this.error = error;
    }

    /** Returns the stable machine-readable error. */
    public RatosError error() {
        return error;
    }
}
