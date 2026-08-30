package io.github.endeavoury.ratatoskr.internal;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.SymbolLookup;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

final class NativeLibrary {
    private static final String PATH_PROPERTY = "ratatoskr.library.path";

    private NativeLibrary() {}

    static SymbolLookup load() {
        if (java.lang.foreign.ValueLayout.ADDRESS.byteSize() != Long.BYTES) {
            throw new UnsupportedOperationException("Ratatoskr Java currently supports 64-bit JVMs only");
        }

        String explicitPath = System.getProperty(PATH_PROPERTY);
        if (explicitPath != null && !explicitPath.isBlank()) {
            Path path = Path.of(explicitPath).toAbsolutePath().normalize();
            if (!Files.isRegularFile(path)) {
                throw new IllegalStateException("Native Ratatoskr library does not exist: " + path);
            }
            return SymbolLookup.libraryLookup(path, Arena.global());
        }

        String platform = platform();
        String fileName = nativeFileName();
        String resource = "/META-INF/native/" + platform + "/" + fileName;
        List<RuntimeException> failures = new ArrayList<>();
        try (InputStream input = NativeLibrary.class.getResourceAsStream(resource)) {
            if (input != null) {
                Path extracted = Files.createTempFile("ratatoskr-", "-" + fileName);
                Files.copy(input, extracted, StandardCopyOption.REPLACE_EXISTING);
                extracted.toFile().deleteOnExit();
                return SymbolLookup.libraryLookup(extracted, Arena.global());
            }
        } catch (IOException exception) {
            failures.add(new IllegalStateException("Could not extract " + resource, exception));
        }

        for (String name : List.of("ratatoskr", System.mapLibraryName("ratatoskr"))) {
            try {
                return SymbolLookup.libraryLookup(name, Arena.global());
            } catch (IllegalArgumentException | UnsatisfiedLinkError exception) {
                failures.add(new IllegalStateException("Could not load native library " + name, exception));
            }
        }

        IllegalStateException error = new IllegalStateException(
            "No Ratatoskr native library for " + platform
                + "; set -D" + PATH_PROPERTY + "=/absolute/path/to/" + fileName);
        failures.forEach(error::addSuppressed);
        throw error;
    }

    private static String platform() {
        String osName = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        String architecture = System.getProperty("os.arch", "").toLowerCase(Locale.ROOT);
        String os;
        if (osName.contains("linux")) os = "linux";
        else if (osName.contains("mac") || osName.contains("darwin")) os = "macos";
        else if (osName.contains("windows")) os = "windows";
        else throw new UnsupportedOperationException("Unsupported operating system: " + osName);

        String arch;
        if (architecture.equals("amd64") || architecture.equals("x86_64")) arch = "x86_64";
        else if (architecture.equals("aarch64") || architecture.equals("arm64")) arch = "aarch64";
        else throw new UnsupportedOperationException("Unsupported architecture: " + architecture);
        return os + "-" + arch;
    }

    private static String nativeFileName() {
        String osName = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        if (osName.contains("windows")) return "ratatoskr.dll";
        if (osName.contains("mac") || osName.contains("darwin")) return "libratatoskr.dylib";
        return "libratatoskr.so";
    }
}
