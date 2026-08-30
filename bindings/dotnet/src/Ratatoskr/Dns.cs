using System.Runtime.InteropServices;

namespace Ratatoskr;

public enum RatosError
{
    Ok = 0, Generic = 1, InvalidArgument = 2, OutOfMemory = 3, Timeout = 4,
    Network = 5, Protocol = 6, Dns = 7, NotFound = 8, Unsupported = 9,
    PermissionDenied = 10
}

public enum DnsRecordType : ushort
{
    A = 1, NS = 2, MD = 3, MF = 4, CNAME = 5, SOA = 6, MB = 7, MG = 8,
    MR = 9, NULL = 10, WKS = 11, PTR = 12, HINFO = 13, MINFO = 14, MX = 15, TXT = 16,
    AAAA = 28, SRV = 33, NAPTR = 35, CAA = 257
}

public enum DnsSection { Answer = 1, Authority = 2, Additional = 3 }

public sealed class RatatoskrException : Exception
{
    public RatatoskrException(RatosError error, string message) : base(message) => Error = error;
    public RatosError Error { get; }
}

public sealed record DnsRecord(
    ushort TypeCode,
    DnsSection Section,
    string Name,
    uint TimeToLive,
    string Text,
    ReadOnlyMemory<byte> RawData,
    IReadOnlyList<ushort> UInt16Fields,
    IReadOnlyList<uint> UInt32Fields,
    IReadOnlyList<string> StringFields)
{
    public DnsRecordType Type => (DnsRecordType)TypeCode;
    public override string ToString() => Text;
}

public sealed class DnsResult
{
    internal DnsResult(string queryName, DnsRecordType queryType, string server, ushort transactionId,
        byte responseCode, bool authoritative, bool truncated, bool recursionDesired,
        bool recursionAvailable, bool authenticData, bool checkingDisabled, IReadOnlyList<DnsRecord> records)
    {
        QueryName = queryName; QueryType = queryType; Server = server; TransactionId = transactionId;
        ResponseCode = responseCode; Authoritative = authoritative; Truncated = truncated;
        RecursionDesired = recursionDesired; RecursionAvailable = recursionAvailable;
        AuthenticData = authenticData; CheckingDisabled = checkingDisabled; Records = records;
    }
    public string QueryName { get; }
    public DnsRecordType QueryType { get; }
    public string Server { get; }
    public ushort TransactionId { get; }
    public byte ResponseCode { get; }
    public bool Authoritative { get; }
    public bool Truncated { get; }
    public bool RecursionDesired { get; }
    public bool RecursionAvailable { get; }
    public bool AuthenticData { get; }
    public bool CheckingDisabled { get; }
    public IReadOnlyList<DnsRecord> Records { get; }
    public IEnumerable<DnsRecord> Answers => Records.Where(record => record.Section == DnsSection.Answer);
}

public sealed class DnsClientOptions
{
    public string? Server { get; set; }
    public ushort Port { get; set; } = 53;
    public TimeSpan Timeout { get; set; } = TimeSpan.FromSeconds(5);
    public bool RecursionDesired { get; set; } = true;
}

public sealed class DnsClient
{
    private readonly DnsClientOptions options;
    public DnsClient() : this(new DnsClientOptions()) { }
    public DnsClient(DnsClientOptions options) => this.options = options ?? throw new ArgumentNullException(nameof(options));
    public DnsResult Query(string name, DnsRecordType type = DnsRecordType.A) => Dns.Query(name, type, options);
    public Task<DnsResult> QueryAsync(string name, DnsRecordType type = DnsRecordType.A,
        CancellationToken cancellationToken = default) => Dns.QueryAsync(name, type, options, cancellationToken);
}

public static class Dns
{
    private const uint SupportedAbiVersion = 1;
    public static uint NativeAbiVersion => Native.ratos_abi_version();

    public static DnsResult Query(string name, DnsRecordType type = DnsRecordType.A) =>
        Query(name, type, new DnsClientOptions());

    internal static DnsResult Query(string name, DnsRecordType type, DnsClientOptions managedOptions)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(name);
        if (NativeAbiVersion != SupportedAbiVersion)
            throw new NotSupportedException($"Ratatoskr native ABI {NativeAbiVersion} is not supported; expected {SupportedAbiVersion}.");
        if (managedOptions.Timeout <= TimeSpan.Zero || managedOptions.Timeout.TotalMilliseconds > uint.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(managedOptions), "Timeout must be positive and fit in milliseconds.");
        using var context = NativeContextHandle.Create();
        IntPtr server = managedOptions.Server is null ? IntPtr.Zero : Marshal.StringToCoTaskMemUTF8(managedOptions.Server);
        try
        {
            var options = new NativeDnsOptions
            {
                StructSize = (uint)Marshal.SizeOf<NativeDnsOptions>(), Server = server,
                Port = managedOptions.Port, Type = type,
                TimeoutMilliseconds = checked((uint)managedOptions.Timeout.TotalMilliseconds),
                RecursionDesired = managedOptions.RecursionDesired ? (byte)1 : (byte)0
            };
            RatosError error = Native.ratos_dns_query(context, name, in options, out IntPtr resultPointer);
            if (error != RatosError.Ok)
                throw new RatatoskrException(error, Message(context, error));
            using var result = new NativeDnsResultHandle(resultPointer);
            return CopyResult(result);
        }
        finally { if (server != IntPtr.Zero) Marshal.FreeCoTaskMem(server); }
    }

    public static Task<DnsResult> QueryAsync(string name, DnsRecordType type = DnsRecordType.A,
        CancellationToken cancellationToken = default) =>
        QueryAsync(name, type, new DnsClientOptions(), cancellationToken);

    internal static Task<DnsResult> QueryAsync(string name, DnsRecordType type,
        DnsClientOptions options, CancellationToken cancellationToken) =>
        Task.Run(() => { cancellationToken.ThrowIfCancellationRequested(); var value = Query(name, type, options); cancellationToken.ThrowIfCancellationRequested(); return value; }, cancellationToken);

    private static string Message(NativeContextHandle context, RatosError error)
    {
        string summary = PtrString(Native.ratos_error_string(error));
        string detail = PtrString(Native.ratos_context_error(context));
        return detail.Length == 0 ? summary : $"{summary}: {detail}";
    }

    private static DnsResult CopyResult(NativeDnsResultHandle result)
    {
        var records = new List<DnsRecord>();
        nuint count = Native.ratos_dns_result_count(result);
        for (nuint index = 0; index < count; index++)
        {
            IntPtr record = Native.ratos_dns_result_record(result, index);
            nuint rawLength;
            IntPtr rawPointer = Native.ratos_dns_record_raw_data(record, out rawLength);
            byte[] raw = new byte[checked((int)rawLength)];
            if (raw.Length != 0) Marshal.Copy(rawPointer, raw, 0, raw.Length);
            var values16 = new List<ushort>();
            for (nuint field = 0; Native.ratos_dns_record_uint16(record, field, out ushort value) != 0; field++) values16.Add(value);
            var values32 = new List<uint>();
            for (nuint field = 0; Native.ratos_dns_record_uint32(record, field, out uint value) != 0; field++) values32.Add(value);
            var strings = new List<string>();
            nuint stringCount = Native.ratos_dns_record_string_count(record);
            for (nuint field = 0; field < stringCount; field++) strings.Add(PtrString(Native.ratos_dns_record_string(record, field)));
            records.Add(new DnsRecord(Native.ratos_dns_record_type_code(record),
                (DnsSection)Native.ratos_dns_record_section(record), PtrString(Native.ratos_dns_record_name(record)),
                Native.ratos_dns_record_ttl(record), PtrString(Native.ratos_dns_record_text(record)), raw,
                values16.AsReadOnly(), values32.AsReadOnly(), strings.AsReadOnly()));
        }
        return new DnsResult(PtrString(Native.ratos_dns_result_query_name(result)),
            Native.ratos_dns_result_query_type(result), PtrString(Native.ratos_dns_result_server(result)),
            Native.ratos_dns_result_transaction_id(result), Native.ratos_dns_result_rcode(result),
            Native.ratos_dns_result_authoritative(result) != 0, Native.ratos_dns_result_truncated(result) != 0,
            Native.ratos_dns_result_recursion_desired(result) != 0, Native.ratos_dns_result_recursion_available(result) != 0,
            Native.ratos_dns_result_authentic_data(result) != 0, Native.ratos_dns_result_checking_disabled(result) != 0,
            records.AsReadOnly());
    }

    private static string PtrString(IntPtr pointer) => pointer == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUTF8(pointer) ?? string.Empty;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeDnsOptions
{
    internal uint StructSize;
    internal IntPtr Server;
    internal ushort Port;
    internal DnsRecordType Type;
    internal uint TimeoutMilliseconds;
    internal byte RecursionDesired;
    private fixed byte reserved[7];
}

internal sealed class NativeContextHandle : SafeHandle
{
    private NativeContextHandle() : base(IntPtr.Zero, true) { }
    internal static NativeContextHandle Create()
    {
        var handle = new NativeContextHandle(); handle.SetHandle(Native.ratos_context_create());
        if (handle.IsInvalid) { handle.Dispose(); throw new OutOfMemoryException(); }
        return handle;
    }
    public override bool IsInvalid => handle == IntPtr.Zero;
    protected override bool ReleaseHandle() { Native.ratos_context_destroy(handle); return true; }
}

internal sealed class NativeDnsResultHandle : SafeHandle
{
    internal NativeDnsResultHandle(IntPtr pointer) : base(IntPtr.Zero, true) => SetHandle(pointer);
    public override bool IsInvalid => handle == IntPtr.Zero;
    protected override bool ReleaseHandle() { Native.ratos_dns_result_destroy(handle); return true; }
}

internal static partial class Native
{
    private const string Library = "ratatoskr";
    [LibraryImport(Library)] internal static partial uint ratos_abi_version();
    [LibraryImport(Library)] internal static partial IntPtr ratos_context_create();
    [LibraryImport(Library)] internal static partial void ratos_context_destroy(IntPtr context);
    [LibraryImport(Library)] internal static partial IntPtr ratos_context_error(NativeContextHandle context);
    [LibraryImport(Library)] internal static partial IntPtr ratos_error_string(RatosError error);
    [LibraryImport(Library, StringMarshalling = StringMarshalling.Utf8)]
    internal static partial RatosError ratos_dns_query(NativeContextHandle context, string name,
        in NativeDnsOptions options, out IntPtr result);
    [LibraryImport(Library)] internal static partial void ratos_dns_result_destroy(IntPtr result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_rcode(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial ushort ratos_dns_result_transaction_id(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_authoritative(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_truncated(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_recursion_desired(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_recursion_available(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_authentic_data(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial byte ratos_dns_result_checking_disabled(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_result_server(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_result_query_name(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial DnsRecordType ratos_dns_result_query_type(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial nuint ratos_dns_result_count(NativeDnsResultHandle result);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_result_record(NativeDnsResultHandle result, nuint index);
    [LibraryImport(Library)] internal static partial ushort ratos_dns_record_type_code(IntPtr record);
    [LibraryImport(Library)] internal static partial byte ratos_dns_record_section(IntPtr record);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_record_name(IntPtr record);
    [LibraryImport(Library)] internal static partial uint ratos_dns_record_ttl(IntPtr record);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_record_raw_data(IntPtr record, out nuint length);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_record_text(IntPtr record);
    [LibraryImport(Library)] internal static partial int ratos_dns_record_uint16(IntPtr record, nuint index, out ushort value);
    [LibraryImport(Library)] internal static partial int ratos_dns_record_uint32(IntPtr record, nuint index, out uint value);
    [LibraryImport(Library)] internal static partial nuint ratos_dns_record_string_count(IntPtr record);
    [LibraryImport(Library)] internal static partial IntPtr ratos_dns_record_string(IntPtr record, nuint index);
}
