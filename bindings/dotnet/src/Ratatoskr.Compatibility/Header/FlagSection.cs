using System;
using System.Linq;

namespace DNS.Client;

/// <summary>
/// 16 bit flag byte
/// 
///   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
/// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
/// |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
/// </summary>
public class FlagSection
{
    private readonly FlagQrField qr;
    private readonly FlagOpcodeField opcode;
    private readonly FlagAuthoritativeAnswerField aa;
    private readonly TruncationField tc;
    private readonly RecursionDesiredField rd;
    private readonly RecursionAvailableField ra;

    public FlagSection(FlagQr qr, FlagOpcode opcode, FlagAuthoritativeAnswer aa, Truncation tc, RecursionDesired rd, RecursionAvailable ra)
    {
        this.qr = new FlagQrField(qr);
        this.opcode = new FlagOpcodeField(opcode);
        this.aa = new FlagAuthoritativeAnswerField(aa);
        this.tc = new TruncationField(tc);
        this.rd = new RecursionDesiredField(rd);
        this.ra = new RecursionAvailableField(ra);
    }

    public byte[] ToByteArray()
    {
        var flag0 = new byte();
        flag0 = flag0.SetBitValueAt(7, 1, this.qr.GetByte());
        flag0 = flag0.SetBitValueAt(3, 4, this.opcode.GetByte());
        flag0 = flag0.SetBitValueAt(2, 1, this.aa.GetByte());
        flag0 = flag0.SetBitValueAt(1, 1, this.tc.GetByte());
        flag0 = flag0.SetBitValueAt(0, 1, this.rd.GetByte());

        var flag1 = new byte();
        flag1 = flag1.SetBitValueAt(7, 1, this.ra.GetByte());
        flag1 = flag1.SetBitValueAt(6, 1, 0);
        flag1 = flag1.SetBitValueAt(5, 1, 0);
        flag1 = flag1.SetBitValueAt(4, 1, 0);
        flag1 = flag1.SetBitValueAt(0, 4, 0);

        return new byte[2] { flag0 , flag1 };
    }
}
