using System.Collections.ObjectModel;

namespace DNS.Client;

public enum DnsOpCode : byte
{
    Query = 0,
    InverseQuery = 1,
    Status = 2
}

public enum DnsResponseCode : byte
{
    NoError = 0,
    FormatError = 1,
    ServerFailure = 2,
    NameError = 3,
    NotImplemented = 4,
    Refused = 5
}

public enum DnsTransport
{
    Udp,
    Tcp
}

public sealed class DnsHeader
{
    public ushort Id { get; set; }
    public bool IsResponse { get; set; }
    public DnsOpCode OpCode { get; set; }
    public bool IsAuthoritativeAnswer { get; set; }
    public bool IsTruncated { get; set; }
    public bool RecursionDesired { get; set; }
    public bool RecursionAvailable { get; set; }
    public byte Reserved { get; set; }
    public DnsResponseCode ResponseCode { get; set; }
    public ushort QuestionCount { get; internal set; }
    public ushort AnswerCount { get; internal set; }
    public ushort AuthorityCount { get; internal set; }
    public ushort AdditionalCount { get; internal set; }
}

public sealed class DnsQuestion
{
    public DnsQuestion(string name, QuestionType type = QuestionType.A,
        QuestionClass @class = QuestionClass.IN)
    {
        Name = name ?? throw new ArgumentNullException(nameof(name));
        Type = type;
        Class = @class;
    }

    public string Name { get; }
    public QuestionType Type { get; }
    public QuestionClass Class { get; }
}

public sealed class DnsMessage
{
    public DnsMessage() : this(new DnsHeader()) { }

    public DnsMessage(DnsHeader header)
    {
        Header = header ?? throw new ArgumentNullException(nameof(header));
    }

    public DnsHeader Header { get; }
    public IList<DnsQuestion> Questions { get; } = new List<DnsQuestion>();
    public IList<DnsResourceRecord> Answers { get; } = new List<DnsResourceRecord>();
    public IList<DnsResourceRecord> Authorities { get; } = new List<DnsResourceRecord>();
    public IList<DnsResourceRecord> Additionals { get; } = new List<DnsResourceRecord>();

    public static DnsMessage CreateQuery(ushort id, string name,
        QuestionType type = QuestionType.A, QuestionClass @class = QuestionClass.IN,
        bool recursionDesired = true)
    {
        var message = new DnsMessage(new DnsHeader
        {
            Id = id,
            OpCode = DnsOpCode.Query,
            RecursionDesired = recursionDesired
        });
        message.Questions.Add(new DnsQuestion(name, type, @class));
        return message;
    }

    public byte[] ToArray(bool compressNames = true) => DnsMessageCodec.Write(this, compressNames);
    public static DnsMessage Parse(ReadOnlySpan<byte> data) => DnsMessageCodec.Read(data);
}

public sealed class DnsProtocolException : Exception
{
    public DnsProtocolException(string message) : base(message) { }
    public DnsProtocolException(string message, Exception innerException) : base(message, innerException) { }
}

public sealed class DnsResponseException : Exception
{
    public DnsResponseException(DnsResponseCode responseCode)
        : base($"The DNS server returned {responseCode} ({(byte)responseCode}).")
    {
        ResponseCode = responseCode;
    }

    public DnsResponseCode ResponseCode { get; }
}

public sealed class DnsMessageCollection : ReadOnlyCollection<DnsMessage>
{
    internal DnsMessageCollection(IList<DnsMessage> messages) : base(messages) { }
}
