using System.Net;

namespace DNS.Client.Tests;

[TestClass]
public class MessageCodecTests
{
    [TestMethod]
    public void StandardQueryHasRfc1035WireFormat()
    {
        byte[] expected = {
            0xa0, 0x5c, 0x01, 0x00,
            0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x06, (byte)'g', (byte)'o', (byte)'o', (byte)'g', (byte)'l', (byte)'e',
            0x02, (byte)'n', (byte)'l', 0x00,
            0x00, 0x01, 0x00, 0x01
        };

        byte[] actual = DnsMessage.CreateQuery(0xa05c, "google.nl").ToArray();

        CollectionAssert.AreEqual(expected, actual);
    }

    [TestMethod]
    public void ParsesCompressedResponse()
    {
        byte[] response = {
            0x12, 0x34, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
            0x03, (byte)'w', (byte)'w', (byte)'w', 0x07, (byte)'e', (byte)'x', (byte)'a',
            (byte)'m', (byte)'p', (byte)'l', (byte)'e', 0x03, (byte)'c', (byte)'o', (byte)'m', 0x00,
            0x00, 0x01, 0x00, 0x01,
            0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x04,
            192, 0, 2, 1
        };

        DnsMessage message = DnsMessage.Parse(response);

        Assert.IsTrue(message.Header.IsResponse);
        Assert.AreEqual(DnsResponseCode.NoError, message.Header.ResponseCode);
        Assert.AreEqual("www.example.com", message.Answers[0].Name);
        Assert.AreEqual(IPAddress.Parse("192.0.2.1"), ((ARecordData)message.Answers[0].Data).Address);
    }

    [TestMethod]
    public void RoundTripsEveryRfc1035RdataShape()
    {
        var message = new DnsMessage(new DnsHeader { Id = 7, IsResponse = true });
        Add(QuestionType.A, new ARecordData(IPAddress.Parse("192.0.2.3")));
        foreach (QuestionType type in new[] { QuestionType.CNAME, QuestionType.MB, QuestionType.MD,
            QuestionType.MF, QuestionType.MG, QuestionType.MR, QuestionType.NS, QuestionType.PTR })
            Add(type, new NameRecordData("target.example"));
        Add(QuestionType.HINFO, new HInfoRecordData("x86_64", "UNIX"));
        Add(QuestionType.MINFO, new MInfoRecordData("admin.example", "errors.example"));
        Add(QuestionType.MX, new MxRecordData(10, "mail.example"));
        Add(QuestionType.SOA, new SoaRecordData("ns.example", "hostmaster.example", 1, 2, 3, 4, 5));
        Add(QuestionType.TXT, new TxtRecordData("first", "second"));
        Add(QuestionType.WKS, new WksRecordData(IPAddress.Parse("192.0.2.4"), 6, new byte[] { 0x80 }));
        Add(QuestionType.NULL, new RawRecordData(new byte[] { 1, 2, 3 }));

        DnsMessage parsed = DnsMessage.Parse(message.ToArray());

        Assert.AreEqual(message.Answers.Count, parsed.Answers.Count);
        Assert.AreEqual("mail.example", ((MxRecordData)parsed.Answers.Single(r => r.Type == QuestionType.MX).Data).Exchange);
        Assert.AreEqual(2, ((TxtRecordData)parsed.Answers.Single(r => r.Type == QuestionType.TXT).Data).Strings.Count);

        void Add(QuestionType type, DnsRecordData data) =>
            message.Answers.Add(new DnsResourceRecord("owner.example", type, QuestionClass.IN, 300, data));
    }

    [TestMethod]
    public void RejectsCompressionPointerLoop()
    {
        byte[] invalid = {
            0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0,
            0xc0, 0x0c, 0, 1, 0, 1
        };

        Assert.ThrowsExactly<DnsProtocolException>(() => DnsMessage.Parse(invalid));
    }

    [TestMethod]
    public void RejectsOverlongLabel()
    {
        string name = new string('a', 64) + ".example";
        Assert.ThrowsExactly<ArgumentException>(() => DnsMessage.CreateQuery(1, name).ToArray());
    }

    [TestMethod]
    public void PreservesDnssecHeaderBitsAndZBitSeparately()
    {
        var message = new DnsMessage(new DnsHeader
        {
            Id = 42, IsResponse = true, AuthenticData = true, CheckingDisabled = true
        });
        message.Questions.Add(new DnsQuestion("example.com"));

        DnsHeader header = DnsMessage.Parse(message.ToArray()).Header;

        Assert.IsTrue(header.AuthenticData);
        Assert.IsTrue(header.CheckingDisabled);
        Assert.AreEqual(0, header.Reserved);
    }

    [TestMethod]
    public void RejectsMultipleQuestionsForStandardQuery()
    {
        var message = new DnsMessage(new DnsHeader { Id = 1, OpCode = DnsOpCode.Query });
        message.Questions.Add(new DnsQuestion("one.example"));
        message.Questions.Add(new DnsQuestion("two.example"));
        byte[] wire = message.ToArray();

        Assert.ThrowsExactly<DnsProtocolException>(() => DnsMessage.Parse(wire));
    }

    [TestMethod]
    public void EnforcesDecoderLimitsBeforeAllocatingSections()
    {
        byte[] wire = DnsMessage.CreateQuery(1, "example.com").ToArray();

        Assert.ThrowsExactly<DnsProtocolException>(() => DnsMessageCodec.Read(wire, maxMessageSize: 12));
        Assert.ThrowsExactly<ArgumentOutOfRangeException>(() => DnsMessageCodec.Read(wire, maxRecordCount: 0));
    }

    [TestMethod]
    public void EscapedNamesRoundTripArbitraryLabelOctets()
    {
        const string name = @"a\.b.back\\slash.\000\255";
        DnsMessage parsed = DnsMessage.Parse(DnsMessage.CreateQuery(1, name).ToArray());
        Assert.AreEqual(name, parsed.Questions[0].Name);
    }

    [TestMethod]
    public void CreatesInAddrArpaReverseName()
    {
        Assert.AreEqual("2.0.0.192.in-addr.arpa",
            DnsClient.GetReverseLookupName(IPAddress.Parse("192.0.0.2")));
    }
}
