namespace DNS.Client;

/// <summary>
/// TrunCation - specifies that this message was truncated
/// due to length greater than that permitted on the transmission channel.
/// </summary>
public class TruncationField
{
    private readonly Truncation value;

    public TruncationField(Truncation value)
    {
        this.value = value;
    }

    public byte GetByte()
    {
        return (byte)value;
    }
}
