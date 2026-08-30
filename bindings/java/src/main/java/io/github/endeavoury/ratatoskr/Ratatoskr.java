package io.github.endeavoury.ratatoskr;

import io.github.endeavoury.ratatoskr.internal.NativeBindings;

/** Entry point and native version information for Ratatoskr. */
public final class Ratatoskr {
    private Ratatoskr() {}

    /** Returns a thread-safe DNS client using system resolver defaults. */
    public static DnsClient dns() {
        return new DnsClient();
    }

    /** Returns the loaded native ABI version. */
    public static int abiVersion() {
        return NativeBindings.abiVersion();
    }

    /** Returns the loaded native product version. */
    public static String version() {
        return NativeBindings.versionMajor() + "." + NativeBindings.versionMinor()
            + "." + NativeBindings.versionPatch();
    }
}
