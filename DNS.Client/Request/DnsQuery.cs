using System.Collections.Generic;

namespace DNS.Client;

public class DnsQuery
{
    private readonly DnsMessage message;

    public DnsQuery(DnsQueryRequest request)
    {
        if (request is null) throw new ArgumentNullException(nameof(request));
        message = new DnsMessage(new DnsHeader
        {
            Id = request.TransactionId,
            IsResponse = request.FlagQr == FlagQr.Response,
            OpCode = (DnsOpCode)(byte)request.FlagOpcode,
            IsAuthoritativeAnswer = request.FlagAuthoritativeAnswer == FlagAuthoritativeAnswer.Owner,
            IsTruncated = request.Truncation == Truncation.Truncated,
            RecursionDesired = request.RecursionDesired == RecursionDesired.Desired,
            RecursionAvailable = request.RecursionAvailable == RecursionAvailable.Available,
            ResponseCode = request.ResponseCode
        });
        foreach (Question question in request.Questions ?? throw new ArgumentNullException(nameof(request.Questions)))
            message.Questions.Add(new DnsQuestion(question.Domain, question.Type, question.Class));
        foreach (DnsResourceRecord record in request.Answers) message.Answers.Add(record);
        foreach (DnsResourceRecord record in request.Authorities) message.Authorities.Add(record);
        foreach (DnsResourceRecord record in request.Additionals) message.Additionals.Add(record);
    }

    public byte[] GetBytes()
    {
        return message.ToArray();
    }

    public DnsMessage Message => message;
}
