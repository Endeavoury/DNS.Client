using System.Collections.Concurrent;
using System.Net;

namespace DNS.Client;

/// <summary>Named DNS endpoint used by <see cref="LookupClient"/>.</summary>
public sealed class NameServer
{
    public NameServer(IPAddress address, int port = 53)
    { Address = address ?? throw new ArgumentNullException(nameof(address)); if (port is < 1 or > 65535) throw new ArgumentOutOfRangeException(nameof(port)); Port = port; }
    public IPAddress Address { get; }
    public int Port { get; }
    public IPEndPoint EndPoint => new(Address, Port);
    public static NameServer GooglePublicDns { get; } = new(IPAddress.Parse("8.8.8.8"));
    public static NameServer Cloudflare { get; } = new(IPAddress.Parse("1.1.1.1"));
    public override string ToString() => $"{Address}:{Port}";
}

public sealed class LookupClientOptions
{
    public IReadOnlyList<NameServer>? NameServers { get; set; }
    public TimeSpan Timeout { get; set; } = TimeSpan.FromSeconds(5);
    public int Retries { get; set; } = 2;
    public bool UseTcpFallback { get; set; } = true;
    public bool ThrowDnsErrors { get; set; }
    public bool EnableCache { get; set; } = true;
}

/// <summary>High-level resolver API with typed record helpers and a small TTL cache.</summary>
public sealed class LookupClient
{
    private readonly DnsClient client;
    private readonly LookupClientOptions options;
    private readonly ConcurrentDictionary<string, (DateTimeOffset Expires, DnsMessage Message)> cache = new();

    public LookupClient() : this(new LookupClientOptions()) { }
    public LookupClient(params IPAddress[] nameServers) : this(new LookupClientOptions { NameServers = nameServers.Select(x => new NameServer(x)).ToArray() }) { }
    public LookupClient(LookupClientOptions options)
    {
        this.options = options ?? throw new ArgumentNullException(nameof(options));
        var servers = options.NameServers?.Select(x => x.EndPoint).ToArray();
        client = servers is { Length: > 0 } ? new DnsClient(servers) : new DnsClient();
        client.Timeout = options.Timeout; client.Attempts = options.Retries; client.ThrowOnResponseError = options.ThrowDnsErrors;
    }
    public IReadOnlyList<IPEndPoint> NameServers => client.Servers;
    public LookupClientOptions Options => options;

    public DnsMessage Query(string name, QuestionType type = QuestionType.A, QuestionClass @class = QuestionClass.IN) => QueryAsync(name, type, @class).GetAwaiter().GetResult();
    public async Task<DnsMessage> QueryAsync(string name, QuestionType type = QuestionType.A, QuestionClass @class = QuestionClass.IN, CancellationToken cancellationToken = default)
    {
        string key = $"{name.TrimEnd('.').ToLowerInvariant()}|{(ushort)type}|{(ushort)@class}";
        if (options.EnableCache && cache.TryGetValue(key, out var hit) && hit.Expires > DateTimeOffset.UtcNow) return hit.Message;
        var result = await client.QueryAsync(name, type, @class, cancellationToken).ConfigureAwait(false);
        if (options.EnableCache)
        {
            uint ttl = result.Answers.Count == 0 ? 1u : result.Answers.Min(x => x.TimeToLive);
            cache[key] = (DateTimeOffset.UtcNow.AddSeconds(ttl), result);
        }
        return result;
    }
    public DnsMessage QueryReverse(IPAddress address) => client.QueryReverse(address);
    public Task<DnsMessage> QueryReverseAsync(IPAddress address, CancellationToken cancellationToken = default) => client.QueryReverseAsync(address, cancellationToken);
    public Task<DnsMessageCollection> TransferZoneAsync(string zone, QuestionClass @class = QuestionClass.IN, CancellationToken cancellationToken = default) => client.TransferZoneAsync(zone, @class, cancellationToken);
    public void ClearCache() => cache.Clear();
}

public static class DnsRecordCollectionExtensions
{
    public static IEnumerable<DnsResourceRecord> OfType(this IEnumerable<DnsResourceRecord> records, QuestionType type) => records.Where(x => x.Type == type);
    public static IEnumerable<ARecordData> ARecords(this IEnumerable<DnsResourceRecord> records) => records.OfType(QuestionType.A).Select(x => (ARecordData)x.Data);
    public static IEnumerable<AaaaRecordData> AaaaRecords(this IEnumerable<DnsResourceRecord> records) => records.OfType(QuestionType.AAAA).Select(x => (AaaaRecordData)x.Data);
    public static IEnumerable<NameRecordData> NameRecords(this IEnumerable<DnsResourceRecord> records, QuestionType type) => records.OfType(type).Select(x => (NameRecordData)x.Data);
    public static IEnumerable<SrvRecordData> SrvRecords(this IEnumerable<DnsResourceRecord> records) => records.OfType(QuestionType.SRV).Select(x => (SrvRecordData)x.Data);
    public static IEnumerable<MxRecordData> MxRecords(this IEnumerable<DnsResourceRecord> records) => records.OfType(QuestionType.MX).Select(x => (MxRecordData)x.Data);
    public static IEnumerable<TxtRecordData> TxtRecords(this IEnumerable<DnsResourceRecord> records) => records.OfType(QuestionType.TXT).Select(x => (TxtRecordData)x.Data);
}
