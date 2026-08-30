using System.Net;

namespace DNS.Client;

internal static class NativeDnsAdapter
{
    internal static async Task<DnsMessage> QueryAsync(IReadOnlyList<IPEndPoint> servers,
        TimeSpan timeout, int attempts, bool throwOnResponseError, string name,
        QuestionType type, QuestionClass @class, CancellationToken cancellationToken)
    {
        if (@class != QuestionClass.IN)
            throw new NotSupportedException("The Ratatoskr DNS v1 query ABI supports the Internet class only.");
        Exception? lastError = null;
        for (int attempt = 0; attempt < Math.Max(1, attempts); attempt++)
        {
            IPEndPoint endpoint = servers[attempt % servers.Count];
            try
            {
                var native = new Ratatoskr.DnsClient(new Ratatoskr.DnsClientOptions
                {
                    Server = endpoint.Address.ToString(), Port = checked((ushort)endpoint.Port), Timeout = timeout
                });
                Ratatoskr.DnsResult result = await native.QueryAsync(name,
                    (Ratatoskr.DnsRecordType)(ushort)type, cancellationToken).ConfigureAwait(false);
                DnsMessage message = Convert(result, @class);
                if (throwOnResponseError && message.Header.ResponseCode != DnsResponseCode.NoError)
                    throw new DnsResponseException(message.Header.ResponseCode);
                return message;
            }
            catch (Ratatoskr.RatatoskrException ex) when (ex.Error is Ratatoskr.RatosError.Network or Ratatoskr.RatosError.Timeout)
            {
                lastError = ex.Error == Ratatoskr.RatosError.Timeout
                    ? new TimeoutException(ex.Message, ex)
                    : new IOException(ex.Message, ex);
            }
            catch (Ratatoskr.RatatoskrException ex)
            {
                throw new DnsProtocolException(ex.Message, ex);
            }
        }
        throw new DnsProtocolException("No DNS server returned a valid response.", lastError!);
    }

    private static DnsMessage Convert(Ratatoskr.DnsResult result, QuestionClass @class)
    {
        var message = new DnsMessage(new DnsHeader
        {
            Id = result.TransactionId,
            IsResponse = true,
            IsAuthoritativeAnswer = result.Authoritative,
            IsTruncated = result.Truncated,
            RecursionDesired = result.RecursionDesired,
            RecursionAvailable = result.RecursionAvailable,
            AuthenticData = result.AuthenticData,
            CheckingDisabled = result.CheckingDisabled,
            ResponseCode = (DnsResponseCode)result.ResponseCode
        });
        message.Questions.Add(new DnsQuestion(result.QueryName, (QuestionType)result.QueryType, @class));
        foreach (Ratatoskr.DnsRecord native in result.Records)
        {
            var record = new DnsResourceRecord(native.Name, (QuestionType)native.TypeCode, @class,
                native.TimeToLive, ConvertData(native));
            switch (native.Section)
            {
                case Ratatoskr.DnsSection.Answer: message.Answers.Add(record); break;
                case Ratatoskr.DnsSection.Authority: message.Authorities.Add(record); break;
                case Ratatoskr.DnsSection.Additional: message.Additionals.Add(record); break;
            }
        }
        return message;
    }

    private static DnsRecordData ConvertData(Ratatoskr.DnsRecord record) => (QuestionType)record.TypeCode switch
    {
        QuestionType.A => new ARecordData(IPAddress.Parse(record.Text)),
        QuestionType.AAAA => new AaaaRecordData(IPAddress.Parse(record.Text)),
        QuestionType.NS or QuestionType.MD or QuestionType.MF or QuestionType.CNAME or QuestionType.MB
            or QuestionType.MG or QuestionType.MR or QuestionType.PTR when record.StringFields.Count >= 1 =>
            new NameRecordData(record.StringFields[0]),
        QuestionType.HINFO when record.StringFields.Count >= 2 =>
            new HInfoRecordData(record.StringFields[0], record.StringFields[1]),
        QuestionType.MINFO when record.StringFields.Count >= 2 =>
            new MInfoRecordData(record.StringFields[0], record.StringFields[1]),
        QuestionType.MX when record.UInt16Fields.Count >= 1 && record.StringFields.Count >= 1 =>
            new MxRecordData(record.UInt16Fields[0], record.StringFields[0]),
        QuestionType.TXT => new TxtRecordData(record.StringFields),
        QuestionType.SRV when record.UInt16Fields.Count >= 3 && record.StringFields.Count >= 1 =>
            new SrvRecordData(record.UInt16Fields[0], record.UInt16Fields[1], record.UInt16Fields[2], record.StringFields[0]),
        QuestionType.SOA when record.StringFields.Count >= 2 && record.UInt32Fields.Count >= 5 =>
            new SoaRecordData(record.StringFields[0], record.StringFields[1], record.UInt32Fields[0],
                record.UInt32Fields[1], record.UInt32Fields[2], record.UInt32Fields[3], record.UInt32Fields[4]),
        QuestionType.NAPTR when record.UInt16Fields.Count >= 2 && record.StringFields.Count >= 4 =>
            new NAPTRRecordData(record.UInt16Fields[0], record.UInt16Fields[1], record.StringFields[0],
                record.StringFields[1], record.StringFields[2], record.StringFields[3]),
        QuestionType.CAA when record.UInt16Fields.Count >= 1 && record.StringFields.Count >= 2 =>
            new CaaRecordData(checked((byte)record.UInt16Fields[0]), record.StringFields[0], record.StringFields[1]),
        QuestionType.WKS when record.RawData.Length >= 5 =>
            new WksRecordData(new IPAddress(record.RawData.Span[..4]), record.RawData.Span[4], record.RawData.Span[5..]),
        _ => new RawRecordData(record.RawData.Span)
    };
}
