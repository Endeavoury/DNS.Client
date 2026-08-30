namespace DNS.Client;

/// <summary>
/// A one bit field that specifies whether this message is a query(0), or a response(1).
/// </summary>
public class FlagQrField
{
    private FlagQr value;

    public FlagQr Value => value;

    public FlagQrField(FlagQr value)
    {
        this.value = value;
    }

    public byte GetByte()
    {
        return (byte)value;
    }
}
