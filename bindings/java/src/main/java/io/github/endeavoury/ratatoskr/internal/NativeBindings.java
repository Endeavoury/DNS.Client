package io.github.endeavoury.ratatoskr.internal;

import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;

/** Internal, low-level projection of ABI 1. Not part of the supported Java API. */
public final class NativeBindings {
    private static final SymbolLookup LOOKUP = NativeLibrary.load();
    private static final Linker LINKER = Linker.nativeLinker();

    private static final MethodHandle ABI_VERSION = bind("ratos_abi_version", FunctionDescriptor.of(ValueLayout.JAVA_INT));
    private static final MethodHandle VERSION_MAJOR = bind("ratos_version_major", FunctionDescriptor.of(ValueLayout.JAVA_INT));
    private static final MethodHandle VERSION_MINOR = bind("ratos_version_minor", FunctionDescriptor.of(ValueLayout.JAVA_INT));
    private static final MethodHandle VERSION_PATCH = bind("ratos_version_patch", FunctionDescriptor.of(ValueLayout.JAVA_INT));
    private static final MethodHandle CONTEXT_CREATE = bind("ratos_context_create", FunctionDescriptor.of(ValueLayout.ADDRESS));
    private static final MethodHandle CONTEXT_DESTROY = bind("ratos_context_destroy", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
    private static final MethodHandle CONTEXT_ERROR = bind("ratos_context_error", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
    private static final MethodHandle ERROR_STRING = bind("ratos_error_string", FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.JAVA_INT));
    private static final MethodHandle OPTIONS_INIT = bind("ratos_dns_query_options_init", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
    private static final MethodHandle DNS_QUERY = bind("ratos_dns_query", FunctionDescriptor.of(ValueLayout.JAVA_INT,
        ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS));
    private static final MethodHandle RESULT_DESTROY = bind("ratos_dns_result_destroy", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
    private static final MethodHandle RESULT_RCODE = unary("ratos_dns_result_rcode", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_ID = unary("ratos_dns_result_transaction_id", ValueLayout.JAVA_SHORT);
    private static final MethodHandle RESULT_AUTHORITATIVE = unary("ratos_dns_result_authoritative", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_TRUNCATED = unary("ratos_dns_result_truncated", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_RD = unary("ratos_dns_result_recursion_desired", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_RA = unary("ratos_dns_result_recursion_available", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_AD = unary("ratos_dns_result_authentic_data", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_CD = unary("ratos_dns_result_checking_disabled", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RESULT_SERVER = unary("ratos_dns_result_server", ValueLayout.ADDRESS);
    private static final MethodHandle RESULT_QUERY_NAME = unary("ratos_dns_result_query_name", ValueLayout.ADDRESS);
    private static final MethodHandle RESULT_QUERY_TYPE = unary("ratos_dns_result_query_type", ValueLayout.JAVA_SHORT);
    private static final MethodHandle RESULT_COUNT = unary("ratos_dns_result_count", ValueLayout.JAVA_LONG);
    private static final MethodHandle RESULT_RECORD = bind("ratos_dns_result_record", FunctionDescriptor.of(ValueLayout.ADDRESS,
        ValueLayout.ADDRESS, ValueLayout.JAVA_LONG));
    private static final MethodHandle RECORD_TYPE = unary("ratos_dns_record_type_code", ValueLayout.JAVA_SHORT);
    private static final MethodHandle RECORD_SECTION = unary("ratos_dns_record_section", ValueLayout.JAVA_BYTE);
    private static final MethodHandle RECORD_NAME = unary("ratos_dns_record_name", ValueLayout.ADDRESS);
    private static final MethodHandle RECORD_TTL = unary("ratos_dns_record_ttl", ValueLayout.JAVA_INT);
    private static final MethodHandle RECORD_RAW = bind("ratos_dns_record_raw_data", FunctionDescriptor.of(ValueLayout.ADDRESS,
        ValueLayout.ADDRESS, ValueLayout.ADDRESS));
    private static final MethodHandle RECORD_TEXT = unary("ratos_dns_record_text", ValueLayout.ADDRESS);
    private static final MethodHandle RECORD_UINT16 = bind("ratos_dns_record_uint16", FunctionDescriptor.of(ValueLayout.JAVA_INT,
        ValueLayout.ADDRESS, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
    private static final MethodHandle RECORD_UINT32 = bind("ratos_dns_record_uint32", FunctionDescriptor.of(ValueLayout.JAVA_INT,
        ValueLayout.ADDRESS, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS));
    private static final MethodHandle RECORD_STRING_COUNT = unary("ratos_dns_record_string_count", ValueLayout.JAVA_LONG);
    private static final MethodHandle RECORD_STRING = bind("ratos_dns_record_string", FunctionDescriptor.of(ValueLayout.ADDRESS,
        ValueLayout.ADDRESS, ValueLayout.JAVA_LONG));
    private static final MethodHandle RCODE_STRING = bind("ratos_dns_rcode_string", FunctionDescriptor.of(ValueLayout.ADDRESS,
        ValueLayout.JAVA_BYTE));

    private NativeBindings() {}

    public static int abiVersion() { return (int) invoke(ABI_VERSION); }
    public static int versionMajor() { return (int) invoke(VERSION_MAJOR); }
    public static int versionMinor() { return (int) invoke(VERSION_MINOR); }
    public static int versionPatch() { return (int) invoke(VERSION_PATCH); }
    static MemorySegment contextCreate() { return (MemorySegment) invoke(CONTEXT_CREATE); }
    static void contextDestroy(MemorySegment context) { invoke(CONTEXT_DESTROY, context); }
    static MemorySegment contextError(MemorySegment context) { return (MemorySegment) invoke(CONTEXT_ERROR, context); }
    static MemorySegment errorString(int error) { return (MemorySegment) invoke(ERROR_STRING, error); }
    static void optionsInit(MemorySegment options) { invoke(OPTIONS_INIT, options); }
    static int dnsQuery(MemorySegment context, MemorySegment name, MemorySegment options, MemorySegment output) {
        return (int) invoke(DNS_QUERY, context, name, options, output);
    }
    static void resultDestroy(MemorySegment result) { invoke(RESULT_DESTROY, result); }
    static int resultRcode(MemorySegment result) { return Byte.toUnsignedInt((byte) invoke(RESULT_RCODE, result)); }
    static int resultId(MemorySegment result) { return Short.toUnsignedInt((short) invoke(RESULT_ID, result)); }
    static boolean resultAuthoritative(MemorySegment result) { return flag(RESULT_AUTHORITATIVE, result); }
    static boolean resultTruncated(MemorySegment result) { return flag(RESULT_TRUNCATED, result); }
    static boolean resultRecursionDesired(MemorySegment result) { return flag(RESULT_RD, result); }
    static boolean resultRecursionAvailable(MemorySegment result) { return flag(RESULT_RA, result); }
    static boolean resultAuthenticData(MemorySegment result) { return flag(RESULT_AD, result); }
    static boolean resultCheckingDisabled(MemorySegment result) { return flag(RESULT_CD, result); }
    static MemorySegment resultServer(MemorySegment result) { return (MemorySegment) invoke(RESULT_SERVER, result); }
    static MemorySegment resultQueryName(MemorySegment result) { return (MemorySegment) invoke(RESULT_QUERY_NAME, result); }
    static int resultQueryType(MemorySegment result) { return Short.toUnsignedInt((short) invoke(RESULT_QUERY_TYPE, result)); }
    static long resultCount(MemorySegment result) { return (long) invoke(RESULT_COUNT, result); }
    static MemorySegment resultRecord(MemorySegment result, long index) {
        return (MemorySegment) invoke(RESULT_RECORD, result, index);
    }
    static int recordType(MemorySegment record) { return Short.toUnsignedInt((short) invoke(RECORD_TYPE, record)); }
    static int recordSection(MemorySegment record) { return Byte.toUnsignedInt((byte) invoke(RECORD_SECTION, record)); }
    static MemorySegment recordName(MemorySegment record) { return (MemorySegment) invoke(RECORD_NAME, record); }
    static long recordTtl(MemorySegment record) { return Integer.toUnsignedLong((int) invoke(RECORD_TTL, record)); }
    static MemorySegment recordRaw(MemorySegment record, MemorySegment length) {
        return (MemorySegment) invoke(RECORD_RAW, record, length);
    }
    static MemorySegment recordText(MemorySegment record) { return (MemorySegment) invoke(RECORD_TEXT, record); }
    static boolean recordUInt16(MemorySegment record, long index, MemorySegment output) {
        return (int) invoke(RECORD_UINT16, record, index, output) != 0;
    }
    static boolean recordUInt32(MemorySegment record, long index, MemorySegment output) {
        return (int) invoke(RECORD_UINT32, record, index, output) != 0;
    }
    static long recordStringCount(MemorySegment record) { return (long) invoke(RECORD_STRING_COUNT, record); }
    static MemorySegment recordString(MemorySegment record, long index) {
        return (MemorySegment) invoke(RECORD_STRING, record, index);
    }
    static MemorySegment rcodeString(int code) { return (MemorySegment) invoke(RCODE_STRING, (byte) code); }

    private static boolean flag(MethodHandle handle, MemorySegment value) {
        return (byte) invoke(handle, value) != 0;
    }

    private static MethodHandle unary(String symbol, java.lang.foreign.MemoryLayout result) {
        return bind(symbol, FunctionDescriptor.of(result, ValueLayout.ADDRESS));
    }

    private static MethodHandle bind(String symbol, FunctionDescriptor descriptor) {
        MemorySegment address = LOOKUP.find(symbol)
            .orElseThrow(() -> new UnsatisfiedLinkError("Missing Ratatoskr ABI symbol: " + symbol));
        return LINKER.downcallHandle(address, descriptor);
    }

    private static Object invoke(MethodHandle handle, Object... arguments) {
        try {
            return handle.invokeWithArguments(arguments);
        } catch (Throwable throwable) {
            if (throwable instanceof RuntimeException runtime) throw runtime;
            if (throwable instanceof Error error) throw error;
            throw new IllegalStateException("Native Ratatoskr invocation failed", throwable);
        }
    }
}
