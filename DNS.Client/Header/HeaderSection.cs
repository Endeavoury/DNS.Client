using System.Collections.Generic;

/*
/// The header contains the following fields:
///                                 1  1  1  1  1  1
///   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                      ID                       |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                    QDCOUNT                    |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                    ANCOUNT                    |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                    NSCOUNT                    |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |                    ARCOUNT                    |
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
namespace DNS.Client
{
    /// <summary>
    /// Header section of the the DNS message
    /// </summary>
    public class HeaderSection
    {
        private readonly TransactionId transactionId;
        private readonly FlagSection flagSection;
        private readonly QdCount qdCount;
        private readonly AnCount anCount;
        private readonly NsCount nsCount;
        private readonly ArCount arCount;

        public HeaderSection(ushort transactionID,
            FlagQr qr,
            FlagOpcode opcode,
            FlagAuthoritativeAnswer aa,
            Truncation tc,
            RecursionDesired rd,
            RecursionAvailable ra,
            ushort qdCount,
            ushort anCount,
            ushort nsCount,
            ushort arCount)
        {
            this.transactionId = new TransactionId(transactionID);
            this.flagSection = new FlagSection(qr, opcode, aa, tc, rd, ra);
            this.qdCount = new QdCount(qdCount);
            this.anCount = new AnCount(anCount);
            this.nsCount = new NsCount(nsCount);
            this.arCount = new ArCount(arCount);
        }

        public byte[] GetBytes()
        {
            var header = new List<byte>();
            header.AddRange(this.transactionId.ToByteArray());
            header.AddRange(this.flagSection.ToByteArray());
            header.AddRange(this.qdCount.ToByteArray());
            header.AddRange(this.anCount.ToByteArray());
            header.AddRange(this.nsCount.ToByteArray());
            header.AddRange(this.arCount.ToByteArray());
            return header.ToArray();
        }
    }
}
