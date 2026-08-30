namespace DNS.Client;

/// <summary>
/// A four bit field that specifies kind of query in this message.
/// This value is set by the originator of a query and copied into the response.
/// </summary>
public class FlagOpcodeField
{
    private readonly FlagOpcode value;

    public FlagOpcodeField(FlagOpcode value)
    {
        this.value = value;
    }

    public byte GetByte()
    {
        return (byte)value;
    }
}
