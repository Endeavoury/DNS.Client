using System.Net;

namespace DNS.Client;

public sealed class DnsResourceRecord
{
    public DnsResourceRecord(string name, QuestionType type, QuestionClass @class,
        uint timeToLive, DnsRecordData data)
    {
        Name = name ?? throw new ArgumentNullException(nameof(name));
        if (timeToLive > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(timeToLive), "RFC 1035 TTL is a positive signed 32-bit value.");
        Type = type;
        Class = @class;
        TimeToLive = timeToLive;
        Data = data ?? throw new ArgumentNullException(nameof(data));
    }

    public string Name { get; }
    public QuestionType Type { get; }
    public QuestionClass Class { get; }
    public uint TimeToLive { get; }
    public DnsRecordData Data { get; }
}

public abstract class DnsRecordData { }

public sealed class ARecordData : DnsRecordData
{
    public ARecordData(IPAddress address)
    {
        Address = address ?? throw new ArgumentNullException(nameof(address));
        if (address.AddressFamily != System.Net.Sockets.AddressFamily.InterNetwork)
            throw new ArgumentException("RFC 1035 A records require an IPv4 address.", nameof(address));
    }

    public IPAddress Address { get; }
}

public sealed class AaaaRecordData : DnsRecordData
{
    public AaaaRecordData(IPAddress address)
    {
        Address = address ?? throw new ArgumentNullException(nameof(address));
        if (address.AddressFamily != System.Net.Sockets.AddressFamily.InterNetworkV6)
            throw new ArgumentException("AAAA records require an IPv6 address.", nameof(address));
    }
    public IPAddress Address { get; }
}

public sealed class SrvRecordData : DnsRecordData
{
    public SrvRecordData(ushort priority, ushort weight, ushort port, string target)
    { Priority = priority; Weight = weight; Port = port; Target = target ?? throw new ArgumentNullException(nameof(target)); }
    public ushort Priority { get; }
    public ushort Weight { get; }
    public ushort Port { get; }
    public string Target { get; }
}

public sealed class NAPTRRecordData : DnsRecordData
{
    public NAPTRRecordData(ushort order, ushort preference, string flags, string services, string regexp, string replacement)
    { Order = order; Preference = preference; Flags = flags ?? ""; Services = services ?? ""; Regexp = regexp ?? ""; Replacement = replacement ?? throw new ArgumentNullException(nameof(replacement)); }
    public ushort Order { get; }
    public ushort Preference { get; }
    public string Flags { get; }
    public string Services { get; }
    public string Regexp { get; }
    public string Replacement { get; }
}

public sealed class CaaRecordData : DnsRecordData
{
    public CaaRecordData(byte flags, string tag, string value)
    { Flags = flags; Tag = tag ?? throw new ArgumentNullException(nameof(tag)); Value = value ?? throw new ArgumentNullException(nameof(value)); }
    public byte Flags { get; }
    public string Tag { get; }
    public string Value { get; }
}

/// <summary>RDATA consisting of one domain name: CNAME, MB, MD, MF, MG, MR, NS or PTR.</summary>
public sealed class NameRecordData : DnsRecordData
{
    public NameRecordData(string name) => Name = name ?? throw new ArgumentNullException(nameof(name));
    public string Name { get; }
}

public sealed class HInfoRecordData : DnsRecordData
{
    public HInfoRecordData(string cpu, string operatingSystem)
    {
        Cpu = cpu ?? throw new ArgumentNullException(nameof(cpu));
        OperatingSystem = operatingSystem ?? throw new ArgumentNullException(nameof(operatingSystem));
    }

    public string Cpu { get; }
    public string OperatingSystem { get; }
}

public sealed class MInfoRecordData : DnsRecordData
{
    public MInfoRecordData(string responsibleMailbox, string errorMailbox)
    {
        ResponsibleMailbox = responsibleMailbox ?? throw new ArgumentNullException(nameof(responsibleMailbox));
        ErrorMailbox = errorMailbox ?? throw new ArgumentNullException(nameof(errorMailbox));
    }

    public string ResponsibleMailbox { get; }
    public string ErrorMailbox { get; }
}

public sealed class MxRecordData : DnsRecordData
{
    public MxRecordData(ushort preference, string exchange)
    {
        Preference = preference;
        Exchange = exchange ?? throw new ArgumentNullException(nameof(exchange));
    }

    public ushort Preference { get; }
    public string Exchange { get; }
}

public sealed class SoaRecordData : DnsRecordData
{
    public SoaRecordData(string primaryNameServer, string responsibleMailbox, uint serial,
        uint refresh, uint retry, uint expire, uint minimum)
    {
        PrimaryNameServer = primaryNameServer ?? throw new ArgumentNullException(nameof(primaryNameServer));
        ResponsibleMailbox = responsibleMailbox ?? throw new ArgumentNullException(nameof(responsibleMailbox));
        Serial = serial;
        Refresh = refresh;
        Retry = retry;
        Expire = expire;
        Minimum = minimum;
    }

    public string PrimaryNameServer { get; }
    public string ResponsibleMailbox { get; }
    public uint Serial { get; }
    public uint Refresh { get; }
    public uint Retry { get; }
    public uint Expire { get; }
    public uint Minimum { get; }
}

public sealed class TxtRecordData : DnsRecordData
{
    public TxtRecordData(params string[] strings) : this((IEnumerable<string>)strings) { }

    public TxtRecordData(IEnumerable<string> strings)
    {
        Strings = (strings ?? throw new ArgumentNullException(nameof(strings))).ToArray();
    }

    public IReadOnlyList<string> Strings { get; }
}

public sealed class WksRecordData : DnsRecordData
{
    public WksRecordData(IPAddress address, byte protocol, ReadOnlySpan<byte> bitmap)
    {
        Address = address ?? throw new ArgumentNullException(nameof(address));
        if (address.AddressFamily != System.Net.Sockets.AddressFamily.InterNetwork)
            throw new ArgumentException("RFC 1035 WKS records require an IPv4 address.", nameof(address));
        Protocol = protocol;
        Bitmap = bitmap.ToArray();
    }

    public IPAddress Address { get; }
    public byte Protocol { get; }
    public ReadOnlyMemory<byte> Bitmap { get; }

    public bool ContainsPort(ushort port)
    {
        int octet = port / 8;
        return octet < Bitmap.Length && (Bitmap.Span[octet] & (1 << (7 - port % 8))) != 0;
    }
}

/// <summary>Opaque data used by NULL and unknown/extension record types.</summary>
public sealed class RawRecordData : DnsRecordData
{
    public RawRecordData(ReadOnlySpan<byte> bytes) => Bytes = bytes.ToArray();
    public ReadOnlyMemory<byte> Bytes { get; }
}
