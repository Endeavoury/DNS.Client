using System.Net;

namespace DNS.Client;

public static class DnsMessageCodec
{
    public static byte[] Write(DnsMessage message, bool compressNames = true)
    {
        if (message is null) throw new ArgumentNullException(nameof(message));
        if (message.Header.Reserved != 0)
            throw new ArgumentException("The RFC 1035 Z bit must be zero.", nameof(message));
        if (message.Header.OpCode == DnsOpCode.Query && message.Questions.Count > 1)
            throw new ArgumentException("QDCOUNT greater than one is not valid for OPCODE=QUERY (RFC 9619).", nameof(message));
        ushort qd = Count(message.Questions.Count, "question");
        ushort an = Count(message.Answers.Count, "answer");
        ushort ns = Count(message.Authorities.Count, "authority");
        ushort ar = Count(message.Additionals.Count, "additional");
        message.Header.QuestionCount = qd;
        message.Header.AnswerCount = an;
        message.Header.AuthorityCount = ns;
        message.Header.AdditionalCount = ar;

        var writer = new DnsWireWriter(compressNames);
        writer.UInt16(message.Header.Id);
        ushort flags = (ushort)((message.Header.IsResponse ? 0x8000 : 0)
            | (((ushort)message.Header.OpCode & 0x0f) << 11)
            | (message.Header.IsAuthoritativeAnswer ? 0x0400 : 0)
            | (message.Header.IsTruncated ? 0x0200 : 0)
            | (message.Header.RecursionDesired ? 0x0100 : 0)
            | (message.Header.RecursionAvailable ? 0x0080 : 0)
            | (message.Header.AuthenticData ? 0x0020 : 0)
            | (message.Header.CheckingDisabled ? 0x0010 : 0)
            | ((message.Header.Reserved & 0x01) << 6)
            | ((ushort)message.Header.ResponseCode & 0x0f));
        writer.UInt16(flags);
        writer.UInt16(qd); writer.UInt16(an); writer.UInt16(ns); writer.UInt16(ar);

        foreach (DnsQuestion question in message.Questions)
        {
            writer.Name(question.Name);
            writer.UInt16((ushort)question.Type);
            writer.UInt16((ushort)question.Class);
        }
        WriteRecords(writer, message.Answers);
        WriteRecords(writer, message.Authorities);
        WriteRecords(writer, message.Additionals);
        return writer.ToArray();
    }

    public static DnsMessage Read(ReadOnlySpan<byte> data, int maxMessageSize = 65535, int maxRecordCount = 4096)
    {
        if (maxMessageSize < 12) throw new ArgumentOutOfRangeException(nameof(maxMessageSize));
        if (maxRecordCount < 1) throw new ArgumentOutOfRangeException(nameof(maxRecordCount));
        if (data.Length > maxMessageSize) throw new DnsProtocolException("DNS message exceeds the configured maximum size.");
        if (data.Length < 12) throw new DnsProtocolException("A DNS message must contain a 12-byte header.");
        var reader = new DnsWireReader(data);
        ushort id = reader.UInt16();
        ushort flags = reader.UInt16();
        var header = new DnsHeader
        {
            Id = id,
            IsResponse = (flags & 0x8000) != 0,
            OpCode = (DnsOpCode)((flags >> 11) & 0x0f),
            IsAuthoritativeAnswer = (flags & 0x0400) != 0,
            IsTruncated = (flags & 0x0200) != 0,
            RecursionDesired = (flags & 0x0100) != 0,
            RecursionAvailable = (flags & 0x0080) != 0,
            AuthenticData = (flags & 0x0020) != 0,
            CheckingDisabled = (flags & 0x0010) != 0,
            Reserved = (byte)((flags >> 6) & 0x01),
            ResponseCode = (DnsResponseCode)(flags & 0x0f),
            QuestionCount = reader.UInt16(),
            AnswerCount = reader.UInt16(),
            AuthorityCount = reader.UInt16(),
            AdditionalCount = reader.UInt16()
        };
        // RFC 9619 updates RFC 1035: standard QUERY messages have at most one question.
        if (header.OpCode == DnsOpCode.Query && header.QuestionCount > 1)
            throw new DnsProtocolException("QDCOUNT greater than one is not valid for OPCODE=QUERY (RFC 9619).");
        uint totalRecords = (uint)header.QuestionCount + header.AnswerCount + header.AuthorityCount + header.AdditionalCount;
        if (totalRecords > (uint)maxRecordCount)
            throw new DnsProtocolException("DNS message contains more records than the configured limit.");
        var message = new DnsMessage(header);

        for (int i = 0; i < header.QuestionCount; i++)
            message.Questions.Add(new DnsQuestion(reader.Name(),
                (QuestionType)reader.UInt16(), (QuestionClass)reader.UInt16()));
        ReadRecords(ref reader, header.AnswerCount, message.Answers);
        ReadRecords(ref reader, header.AuthorityCount, message.Authorities);
        ReadRecords(ref reader, header.AdditionalCount, message.Additionals);
        if (reader.Position != reader.Length)
            throw new DnsProtocolException("DNS message contains trailing data not described by its section counts.");
        return message;
    }

    private static void WriteRecords(DnsWireWriter writer, IEnumerable<DnsResourceRecord> records)
    {
        foreach (DnsResourceRecord record in records)
        {
            writer.Name(record.Name);
            writer.UInt16((ushort)record.Type);
            writer.UInt16((ushort)record.Class);
            writer.UInt32(record.TimeToLive);
            int lengthOffset = writer.Position;
            writer.UInt16(0);
            int start = writer.Position;
            WriteRecordData(writer, record.Type, record.Class, record.Data);
            int length = writer.Position - start;
            if (length > ushort.MaxValue) throw new ArgumentException("RDATA cannot exceed 65535 octets.");
            writer.PatchUInt16(lengthOffset, (ushort)length);
        }
    }

    private static void WriteRecordData(DnsWireWriter writer, QuestionType type, QuestionClass @class, DnsRecordData data)
    {
        switch (type)
        {
            case QuestionType.A when @class == QuestionClass.IN && data is ARecordData a:
                writer.Bytes(a.Address.GetAddressBytes());
                return;
            case QuestionType.AAAA when @class == QuestionClass.IN && data is AaaaRecordData aaaa:
                writer.Bytes(aaaa.Address.GetAddressBytes());
                return;
            case QuestionType.CNAME or QuestionType.MB or QuestionType.MD or QuestionType.MF
                or QuestionType.MG or QuestionType.MR or QuestionType.NS or QuestionType.PTR
                when data is NameRecordData name:
                writer.Name(name.Name);
                return;
            case QuestionType.HINFO when data is HInfoRecordData hinfo:
                writer.CharacterString(hinfo.Cpu);
                writer.CharacterString(hinfo.OperatingSystem);
                return;
            case QuestionType.MINFO when data is MInfoRecordData minfo:
                writer.Name(minfo.ResponsibleMailbox);
                writer.Name(minfo.ErrorMailbox);
                return;
            case QuestionType.MX when data is MxRecordData mx:
                writer.UInt16(mx.Preference);
                writer.Name(mx.Exchange);
                return;
            case QuestionType.SRV when data is SrvRecordData srv:
                writer.UInt16(srv.Priority); writer.UInt16(srv.Weight); writer.UInt16(srv.Port); writer.Name(srv.Target);
                return;
            case QuestionType.NAPTR when data is NAPTRRecordData naptr:
                writer.UInt16(naptr.Order); writer.UInt16(naptr.Preference);
                writer.CharacterString(naptr.Flags); writer.CharacterString(naptr.Services); writer.CharacterString(naptr.Regexp); writer.Name(naptr.Replacement);
                return;
            case QuestionType.CAA when data is CaaRecordData caa:
                writer.Byte(caa.Flags); writer.CharacterString(caa.Tag); writer.Bytes(System.Text.Encoding.UTF8.GetBytes(caa.Value));
                return;
            case QuestionType.SOA when data is SoaRecordData soa:
                writer.Name(soa.PrimaryNameServer);
                writer.Name(soa.ResponsibleMailbox);
                writer.UInt32(soa.Serial); writer.UInt32(soa.Refresh); writer.UInt32(soa.Retry);
                writer.UInt32(soa.Expire); writer.UInt32(soa.Minimum);
                return;
            case QuestionType.TXT when data is TxtRecordData txt:
                if (txt.Strings.Count == 0) throw new ArgumentException("TXT RDATA must contain at least one string.");
                foreach (string value in txt.Strings) writer.CharacterString(value);
                return;
            case QuestionType.WKS when @class == QuestionClass.IN && data is WksRecordData wks:
                writer.Bytes(wks.Address.GetAddressBytes());
                writer.Byte(wks.Protocol);
                writer.Bytes(wks.Bitmap.Span);
                return;
            case QuestionType.NULL when data is RawRecordData raw:
                writer.Bytes(raw.Bytes.Span);
                return;
            default:
                if (data is RawRecordData unknown)
                {
                    writer.Bytes(unknown.Bytes.Span);
                    return;
                }
                throw new ArgumentException($"{data.GetType().Name} is not valid RDATA for {type}.");
        }
    }

    private static void ReadRecords(ref DnsWireReader reader, ushort count, IList<DnsResourceRecord> target)
    {
        for (int i = 0; i < count; i++)
        {
            string name = reader.Name();
            var type = (QuestionType)reader.UInt16();
            var @class = (QuestionClass)reader.UInt16();
            uint ttl = reader.UInt32();
            if (ttl > int.MaxValue) throw new DnsProtocolException("RFC 1035 TTL exceeds the positive signed 32-bit range.");
            ushort length = reader.UInt16();
            if (length > reader.Length - reader.Position)
                throw new DnsProtocolException("RDATA extends past the DNS message.");
            int end = reader.Position + length;
            DnsRecordData recordData = ReadRecordData(ref reader, type, @class, end);
            if (reader.Position != end)
                throw new DnsProtocolException($"Invalid RDLENGTH for {type}: expected end {end}, actual {reader.Position}.");
            target.Add(new DnsResourceRecord(name, type, @class, ttl, recordData));
        }
    }

    private static DnsRecordData ReadRecordData(ref DnsWireReader reader, QuestionType type,
        QuestionClass @class, int end)
    {
        switch (type)
        {
            case QuestionType.A when @class == QuestionClass.IN:
                RequireLength(reader, end, 4, type);
                return new ARecordData(new IPAddress(reader.Bytes(4)));
            case QuestionType.AAAA when @class == QuestionClass.IN:
                RequireLength(reader, end, 16, type);
                return new AaaaRecordData(new IPAddress(reader.Bytes(16)));
            case QuestionType.CNAME:
            case QuestionType.MB:
            case QuestionType.MD:
            case QuestionType.MF:
            case QuestionType.MG:
            case QuestionType.MR:
            case QuestionType.NS:
            case QuestionType.PTR:
                return new NameRecordData(reader.Name());
            case QuestionType.HINFO:
                return new HInfoRecordData(reader.CharacterString(end), reader.CharacterString(end));
            case QuestionType.MINFO:
                return new MInfoRecordData(reader.Name(), reader.Name());
            case QuestionType.MX:
                return new MxRecordData(reader.UInt16(), reader.Name());
            case QuestionType.SRV:
                if (end - reader.Position < 7) throw new DnsProtocolException("SRV RDATA is too short.");
                return new SrvRecordData(reader.UInt16(), reader.UInt16(), reader.UInt16(), reader.Name());
            case QuestionType.NAPTR:
                return new NAPTRRecordData(reader.UInt16(), reader.UInt16(), reader.CharacterString(end), reader.CharacterString(end), reader.CharacterString(end), reader.Name());
            case QuestionType.CAA:
                if (end - reader.Position < 2) throw new DnsProtocolException("CAA RDATA is too short.");
                byte caaFlags = reader.Byte();
                string caaTag = reader.CharacterString(end);
                return new CaaRecordData(caaFlags, caaTag, DnsWireWriter.WireEncoding.GetString(reader.Bytes(end - reader.Position)));
            case QuestionType.SOA:
                return new SoaRecordData(reader.Name(), reader.Name(), reader.UInt32(), reader.UInt32(),
                    reader.UInt32(), reader.UInt32(), reader.UInt32());
            case QuestionType.TXT:
                var strings = new List<string>();
                while (reader.Position < end) strings.Add(reader.CharacterString(end));
                if (strings.Count == 0) throw new DnsProtocolException("TXT RDATA must contain at least one string.");
                return new TxtRecordData(strings);
            case QuestionType.WKS when @class == QuestionClass.IN:
                if (end - reader.Position < 5) throw new DnsProtocolException("WKS RDATA must be at least 5 octets.");
                var address = new IPAddress(reader.Bytes(4));
                byte protocol = reader.Byte();
                return new WksRecordData(address, protocol, reader.Bytes(end - reader.Position));
            default:
                return new RawRecordData(reader.Bytes(end - reader.Position));
        }
    }

    private static void RequireLength(DnsWireReader reader, int end, int expected, QuestionType type)
    {
        if (end - reader.Position != expected)
            throw new DnsProtocolException($"{type} RDATA must contain exactly {expected} octets.");
    }

    private static ushort Count(int value, string section) => value <= ushort.MaxValue
        ? (ushort)value
        : throw new ArgumentException($"The {section} section contains more than 65535 entries.");
}
