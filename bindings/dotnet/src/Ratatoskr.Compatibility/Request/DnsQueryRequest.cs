using System.Collections.Generic;

namespace DNS.Client;

public class DnsQueryRequest
{
    public ushort TransactionId { get; set; }
    public FlagQr FlagQr { get; set; }
    public FlagOpcode FlagOpcode { get; set; }
    public FlagAuthoritativeAnswer FlagAuthoritativeAnswer { get;set;}
    public Truncation Truncation { get; set; }
    public RecursionDesired RecursionDesired { get; set; }
    public RecursionAvailable RecursionAvailable { get; set; }
    public DnsResponseCode ResponseCode { get; set; }
    public IEnumerable<Question> Questions { get; set; } = new List<Question>();
    public IEnumerable<DnsResourceRecord> Answers { get; set; } = new List<DnsResourceRecord>();
    public IEnumerable<DnsResourceRecord> Authorities { get; set; } = new List<DnsResourceRecord>();
    public IEnumerable<DnsResourceRecord> Additionals { get; set; } = new List<DnsResourceRecord>();
}
