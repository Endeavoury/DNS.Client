package io.github.endeavoury.ratatoskr.internal;

import io.github.endeavoury.ratatoskr.DnsQueryOptions;
import io.github.endeavoury.ratatoskr.DnsRecord;
import io.github.endeavoury.ratatoskr.DnsRecordType;
import io.github.endeavoury.ratatoskr.DnsResult;
import io.github.endeavoury.ratatoskr.DnsSection;
import io.github.endeavoury.ratatoskr.RatosError;
import io.github.endeavoury.ratatoskr.RatatoskrException;
import java.lang.foreign.Arena;
import java.lang.foreign.MemoryLayout;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.ArrayList;
import java.util.List;

/** Copies native DNS ownership into immutable Java values. */
public final class NativeDns {
    private static final long MAX_NATIVE_STRING_BYTES = 1_048_576;
    private static final MemoryLayout OPTIONS_LAYOUT = MemoryLayout.structLayout(
        ValueLayout.JAVA_INT,
        MemoryLayout.paddingLayout(4),
        ValueLayout.ADDRESS,
        ValueLayout.JAVA_SHORT,
        ValueLayout.JAVA_SHORT,
        ValueLayout.JAVA_INT,
        ValueLayout.JAVA_BYTE,
        MemoryLayout.sequenceLayout(7, ValueLayout.JAVA_BYTE));

    private NativeDns() {}

    public static DnsResult query(String name, DnsRecordType type, DnsQueryOptions settings) {
        if (NativeBindings.abiVersion() != 1) {
            throw new UnsupportedOperationException(
                "Unsupported Ratatoskr native ABI " + NativeBindings.abiVersion() + "; expected 1");
        }
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment context = NativeBindings.contextCreate();
            if (isNull(context)) throw new OutOfMemoryError("Could not create Ratatoskr context");
            try {
                return query(context, arena, name, type, settings);
            } finally {
                NativeBindings.contextDestroy(context);
            }
        }
    }

    private static DnsResult query(MemorySegment context, Arena arena, String name,
                                   DnsRecordType type, DnsQueryOptions settings) {
        MemorySegment nativeOptions = arena.allocate(OPTIONS_LAYOUT);
        NativeBindings.optionsInit(nativeOptions);
        MemorySegment server = settings.server() == null ? MemorySegment.NULL : arena.allocateFrom(settings.server());
        nativeOptions.set(ValueLayout.ADDRESS, 8, server);
        nativeOptions.set(ValueLayout.JAVA_SHORT, 16, (short) settings.port());
        nativeOptions.set(ValueLayout.JAVA_SHORT, 18, (short) type.code());
        nativeOptions.set(ValueLayout.JAVA_INT, 20, (int) settings.timeout().toMillis());
        nativeOptions.set(ValueLayout.JAVA_BYTE, 24, settings.recursionDesired() ? (byte) 1 : (byte) 0);

        MemorySegment output = arena.allocate(ValueLayout.ADDRESS);
        int errorCode = NativeBindings.dnsQuery(context, arena.allocateFrom(name), nativeOptions, output);
        if (errorCode != 0) {
            RatosError error = RatosError.fromCode(errorCode);
            String summary = string(NativeBindings.errorString(errorCode));
            String detail = string(NativeBindings.contextError(context));
            throw new RatatoskrException(error, detail.isEmpty() ? summary : summary + ": " + detail);
        }

        MemorySegment result = output.get(ValueLayout.ADDRESS, 0);
        if (isNull(result)) throw new IllegalStateException("Native query succeeded without a result");
        try {
            return copyResult(result);
        } finally {
            NativeBindings.resultDestroy(result);
        }
    }

    private static DnsResult copyResult(MemorySegment result) {
        int code = NativeBindings.resultRcode(result);
        int queryTypeCode = NativeBindings.resultQueryType(result);
        DnsRecordType queryType = DnsRecordType.fromCode(queryTypeCode)
            .orElseThrow(() -> new IllegalStateException("Native result returned unknown query type " + queryTypeCode));
        long count = NativeBindings.resultCount(result);
        if (count < 0 || count > Integer.MAX_VALUE) {
            throw new IllegalStateException("Native result record count is too large: " + count);
        }
        List<DnsRecord> records = new ArrayList<>((int) count);
        for (long index = 0; index < count; index++) {
            MemorySegment record = NativeBindings.resultRecord(result, index);
            if (isNull(record)) throw new IllegalStateException("Native result contains a null record");
            records.add(copyRecord(record));
        }
        return new DnsResult(
            string(NativeBindings.resultQueryName(result)), queryType,
            string(NativeBindings.resultServer(result)), NativeBindings.resultId(result), code,
            string(NativeBindings.rcodeString(code)), NativeBindings.resultAuthoritative(result),
            NativeBindings.resultTruncated(result), NativeBindings.resultRecursionDesired(result),
            NativeBindings.resultRecursionAvailable(result), NativeBindings.resultAuthenticData(result),
            NativeBindings.resultCheckingDisabled(result), records);
    }

    private static DnsRecord copyRecord(MemorySegment record) {
        try (Arena scratch = Arena.ofConfined()) {
            MemorySegment lengthOutput = scratch.allocate(ValueLayout.JAVA_LONG);
            MemorySegment rawPointer = NativeBindings.recordRaw(record, lengthOutput);
            long length = lengthOutput.get(ValueLayout.JAVA_LONG, 0);
            if (length < 0 || length > Integer.MAX_VALUE) {
                throw new IllegalStateException("Native RDATA is too large: " + length);
            }
            byte[] raw = length == 0 ? new byte[0]
                : rawPointer.reinterpret(length).toArray(ValueLayout.JAVA_BYTE);

            List<Integer> values16 = new ArrayList<>();
            MemorySegment output16 = scratch.allocate(ValueLayout.JAVA_SHORT);
            for (long index = 0; NativeBindings.recordUInt16(record, index, output16); index++) {
                values16.add(Short.toUnsignedInt(output16.get(ValueLayout.JAVA_SHORT, 0)));
            }
            List<Long> values32 = new ArrayList<>();
            MemorySegment output32 = scratch.allocate(ValueLayout.JAVA_INT);
            for (long index = 0; NativeBindings.recordUInt32(record, index, output32); index++) {
                values32.add(Integer.toUnsignedLong(output32.get(ValueLayout.JAVA_INT, 0)));
            }
            long stringCount = NativeBindings.recordStringCount(record);
            if (stringCount < 0 || stringCount > Integer.MAX_VALUE) {
                throw new IllegalStateException("Native string field count is too large: " + stringCount);
            }
            List<String> strings = new ArrayList<>((int) stringCount);
            for (long index = 0; index < stringCount; index++) {
                strings.add(string(NativeBindings.recordString(record, index)));
            }
            return new DnsRecord(NativeBindings.recordType(record),
                DnsSection.fromCode(NativeBindings.recordSection(record)),
                string(NativeBindings.recordName(record)), NativeBindings.recordTtl(record),
                string(NativeBindings.recordText(record)), raw, values16, values32, strings);
        }
    }

    private static String string(MemorySegment pointer) {
        return isNull(pointer) ? "" : pointer.reinterpret(MAX_NATIVE_STRING_BYTES).getString(0);
    }

    private static boolean isNull(MemorySegment pointer) {
        return pointer.address() == 0;
    }
}
