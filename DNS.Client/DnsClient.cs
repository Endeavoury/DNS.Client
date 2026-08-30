using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Security.Cryptography;

namespace DNS.Client;

/// <summary>An RFC 1035 DNS stub client with UDP, TCP fallback, and AXFR support.</summary>
public sealed class DnsClient
{
    private const int DnsPort = 53;
    private const int Rfc1035UdpLimit = 512;
    private readonly IReadOnlyList<IPEndPoint> servers;

    public DnsClient() : this(GetSystemNameServers()) { }

    public DnsClient(IPAddress server, int port = DnsPort)
        : this(new[] { new IPEndPoint(server ?? throw new ArgumentNullException(nameof(server)), port) }) { }

    public DnsClient(IEnumerable<IPEndPoint> servers)
    {
        this.servers = (servers ?? throw new ArgumentNullException(nameof(servers))).ToArray();
        if (this.servers.Count == 0) throw new ArgumentException("At least one DNS server is required.", nameof(servers));
    }

    public IReadOnlyList<IPEndPoint> Servers => servers;
    public TimeSpan Timeout { get; set; } = TimeSpan.FromSeconds(5);
    public int Attempts { get; set; } = 2;
    public bool ThrowOnResponseError { get; set; }

    public DnsMessage Query(string name, QuestionType type = QuestionType.A,
        QuestionClass @class = QuestionClass.IN, CancellationToken cancellationToken = default) =>
        QueryAsync(name, type, @class, cancellationToken).GetAwaiter().GetResult();

    public Task<DnsMessage> QueryAsync(string name, QuestionType type = QuestionType.A,
        QuestionClass @class = QuestionClass.IN, CancellationToken cancellationToken = default)
    {
        var query = DnsMessage.CreateQuery(CreateTransactionId(), name, type, @class);
        return SendAsync(query, cancellationToken);
    }

    public DnsMessage QueryReverse(IPAddress address, CancellationToken cancellationToken = default) =>
        QueryReverseAsync(address, cancellationToken).GetAwaiter().GetResult();

    /// <summary>Queries the RFC 1035 IN-ADDR.ARPA name for an IPv4 address.</summary>
    public Task<DnsMessage> QueryReverseAsync(IPAddress address, CancellationToken cancellationToken = default) =>
        QueryAsync(GetReverseLookupName(address), QuestionType.PTR, QuestionClass.IN, cancellationToken);

    public static string GetReverseLookupName(IPAddress address)
    {
        if (address is null) throw new ArgumentNullException(nameof(address));
        if (address.AddressFamily != AddressFamily.InterNetwork)
            throw new ArgumentException("RFC 1035 IN-ADDR.ARPA reverse lookup requires an IPv4 address.", nameof(address));
        byte[] octets = address.GetAddressBytes();
        return $"{octets[3]}.{octets[2]}.{octets[1]}.{octets[0]}.in-addr.arpa";
    }

    /// <summary>Sends using UDP and retries with TCP when required by RFC 1035.</summary>
    public async Task<DnsMessage> SendAsync(DnsMessage query, CancellationToken cancellationToken = default)
    {
        ValidateQuery(query);
        byte[] payload = query.ToArray();
        if (payload.Length > Rfc1035UdpLimit)
            return await CompleteAsync(SendTcpCoreAsync(query, servers[0], cancellationToken)).ConfigureAwait(false);

        Exception? lastError = null;
        int attempts = Math.Max(1, Attempts);
        for (int attempt = 0; attempt < attempts; attempt++)
        {
            IPEndPoint server = servers[attempt % servers.Count];
            try
            {
                DnsMessage response = await SendUdpAsync(payload, query, server, cancellationToken).ConfigureAwait(false);
                if (response.Header.IsTruncated)
                    response = await SendTcpCoreAsync(query, server, cancellationToken).ConfigureAwait(false);
                CheckResponseCode(response);
                return response;
            }
            catch (Exception ex) when (ex is SocketException or IOException or TimeoutException)
            {
                lastError = ex;
            }
        }
        throw new DnsProtocolException("No DNS server returned a valid response.", lastError!);
    }

    public async Task<DnsMessage> SendTcpAsync(DnsMessage query, CancellationToken cancellationToken = default)
    {
        ValidateQuery(query);
        return await CompleteAsync(SendTcpCoreAsync(query, servers[0], cancellationToken)).ConfigureAwait(false);
    }

    /// <summary>Transfers a zone over TCP through the terminating second SOA record.</summary>
    public async Task<DnsMessageCollection> TransferZoneAsync(string zone,
        QuestionClass @class = QuestionClass.IN, CancellationToken cancellationToken = default)
    {
        var query = DnsMessage.CreateQuery(CreateTransactionId(), zone, QuestionType.AXFR, @class, false);
        byte[] payload = query.ToArray();
        using var timeout = CreateTimeoutToken(cancellationToken);
        using var tcp = new TcpClient(servers[0].AddressFamily);
        await ConnectAsync(tcp, servers[0], timeout.Token).ConfigureAwait(false);
        using NetworkStream stream = tcp.GetStream();
        await WriteFrameAsync(stream, payload, timeout.Token).ConfigureAwait(false);

        var messages = new List<DnsMessage>();
        int soaCount = 0;
        while (soaCount < 2)
        {
            DnsMessage response = DnsMessage.Parse(await ReadFrameAsync(stream, timeout.Token).ConfigureAwait(false));
            ValidateResponse(query, response, allowOmittedQuestion: messages.Count > 0);
            CheckResponseCode(response);
            messages.Add(response);
            soaCount += response.Answers.Count(record => record.Type == QuestionType.SOA);
            if (messages.Count == 1 && (response.Answers.Count == 0 || response.Answers[0].Type != QuestionType.SOA))
                throw new DnsProtocolException("An AXFR response must begin with the zone SOA record.");
        }
        return new DnsMessageCollection(messages);
    }

    [Obsolete("Use SendAsync(new DnsQuery(request).Message) and inspect the returned DnsMessage.")]
    public void Request(DnsQueryRequest request) => SendAsync(new DnsQuery(request).Message).GetAwaiter().GetResult();

    private async Task<DnsMessage> CompleteAsync(Task<DnsMessage> operation)
    {
        DnsMessage response = await operation.ConfigureAwait(false);
        CheckResponseCode(response);
        return response;
    }

    private async Task<DnsMessage> SendUdpAsync(byte[] payload, DnsMessage query, IPEndPoint server,
        CancellationToken cancellationToken)
    {
        using var timeout = CreateTimeoutToken(cancellationToken);
        using var udp = new UdpClient(server.AddressFamily);
        udp.Connect(server);
        try
        {
            await udp.SendAsync(payload, payload.Length).WithCancellation(timeout.Token).ConfigureAwait(false);
            UdpReceiveResult received = await udp.ReceiveAsync().WithCancellation(timeout.Token).ConfigureAwait(false);
            DnsMessage response = DnsMessage.Parse(received.Buffer);
            ValidateResponse(query, response);
            return response;
        }
        catch (OperationCanceledException ex) when (!cancellationToken.IsCancellationRequested)
        { throw new TimeoutException($"DNS request to {server} timed out.", ex); }
    }

    private async Task<DnsMessage> SendTcpCoreAsync(DnsMessage query, IPEndPoint server,
        CancellationToken cancellationToken)
    {
        byte[] payload = query.ToArray();
        using var timeout = CreateTimeoutToken(cancellationToken);
        using var tcp = new TcpClient(server.AddressFamily);
        try
        {
            await ConnectAsync(tcp, server, timeout.Token).ConfigureAwait(false);
            using NetworkStream stream = tcp.GetStream();
            await WriteFrameAsync(stream, payload, timeout.Token).ConfigureAwait(false);
            DnsMessage response = DnsMessage.Parse(await ReadFrameAsync(stream, timeout.Token).ConfigureAwait(false));
            ValidateResponse(query, response);
            return response;
        }
        catch (OperationCanceledException ex) when (!cancellationToken.IsCancellationRequested)
        { throw new TimeoutException($"DNS request to {server} timed out.", ex); }
    }

    private static Task ConnectAsync(TcpClient client, IPEndPoint server, CancellationToken token) =>
        client.ConnectAsync(server.Address, server.Port).WithCancellation(token);

    private static async Task WriteFrameAsync(NetworkStream stream, byte[] payload, CancellationToken token)
    {
        if (payload.Length > ushort.MaxValue) throw new DnsProtocolException("A TCP DNS message cannot exceed 65535 octets.");
        byte[] prefix = { (byte)(payload.Length >> 8), (byte)payload.Length };
        await stream.WriteAsync(prefix.AsMemory(), token).ConfigureAwait(false);
        await stream.WriteAsync(payload.AsMemory(), token).ConfigureAwait(false);
    }

    private static async Task<byte[]> ReadFrameAsync(NetworkStream stream, CancellationToken token)
    {
        byte[] prefix = new byte[2];
        await ReadExactlyAsync(stream, prefix, token).ConfigureAwait(false);
        int length = (prefix[0] << 8) | prefix[1];
        if (length < 12) throw new DnsProtocolException("Invalid TCP DNS message length.");
        byte[] payload = new byte[length];
        await ReadExactlyAsync(stream, payload, token).ConfigureAwait(false);
        return payload;
    }

    private static async Task ReadExactlyAsync(NetworkStream stream, byte[] buffer, CancellationToken token)
    {
        int read = 0;
        while (read < buffer.Length)
        {
            int count = await stream.ReadAsync(buffer.AsMemory(read), token).ConfigureAwait(false);
            if (count == 0) throw new IOException("The DNS TCP connection closed before a complete message arrived.");
            read += count;
        }
    }

    private CancellationTokenSource CreateTimeoutToken(CancellationToken cancellationToken)
    {
        if (Timeout <= TimeSpan.Zero && Timeout != System.Threading.Timeout.InfiniteTimeSpan)
            throw new InvalidOperationException("Timeout must be positive or infinite.");
        var source = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        if (Timeout != System.Threading.Timeout.InfiniteTimeSpan) source.CancelAfter(Timeout);
        return source;
    }

    private void CheckResponseCode(DnsMessage response)
    {
        if (ThrowOnResponseError && response.Header.ResponseCode != DnsResponseCode.NoError)
            throw new DnsResponseException(response.Header.ResponseCode);
    }

    private static void ValidateQuery(DnsMessage query)
    {
        if (query is null) throw new ArgumentNullException(nameof(query));
        if (query.Header.IsResponse) throw new ArgumentException("A client cannot send a response as a query.", nameof(query));
    }

    private static void ValidateResponse(DnsMessage query, DnsMessage response, bool allowOmittedQuestion = false)
    {
        if (!response.Header.IsResponse) throw new DnsProtocolException("Received DNS message is not a response.");
        if (response.Header.Reserved != 0) throw new DnsProtocolException("DNS response has non-zero reserved header bits.");
        if (response.Header.Id != query.Header.Id) throw new DnsProtocolException("DNS response transaction ID does not match the query.");
        if (response.Header.OpCode != query.Header.OpCode) throw new DnsProtocolException("DNS response opcode does not match the query.");
        if (allowOmittedQuestion && response.Questions.Count == 0) return;
        if (response.Questions.Count != query.Questions.Count) throw new DnsProtocolException("DNS response question count does not match the query.");
        for (int i = 0; i < query.Questions.Count; i++)
        {
            DnsQuestion expected = query.Questions[i];
            DnsQuestion actual = response.Questions[i];
            if (!string.Equals(NormalizeName(expected.Name), NormalizeName(actual.Name), StringComparison.OrdinalIgnoreCase)
                || expected.Type != actual.Type || expected.Class != actual.Class)
                throw new DnsProtocolException("DNS response question does not match the query.");
        }
    }

    private static string NormalizeName(string value) => value == "." ? string.Empty : value.TrimEnd('.');
    private static ushort CreateTransactionId()
    {
        byte[] bytes = new byte[2];
        using var generator = RandomNumberGenerator.Create();
        generator.GetBytes(bytes);
        return (ushort)((bytes[0] << 8) | bytes[1]);
    }

    private static IReadOnlyList<IPEndPoint> GetSystemNameServers()
    {
        var addresses = NetworkInterface.GetAllNetworkInterfaces()
            .Where(network => network.OperationalStatus == OperationalStatus.Up)
            .SelectMany(network => network.GetIPProperties().DnsAddresses)
            .Where(address => !address.Equals(IPAddress.Any) && !address.Equals(IPAddress.IPv6Any))
            .Distinct()
            .Select(address => new IPEndPoint(address, DnsPort))
            .ToArray();
        if (addresses.Length == 0)
            throw new InvalidOperationException("No system DNS server was found; construct DnsClient with an explicit server.");
        return addresses;
    }
}
