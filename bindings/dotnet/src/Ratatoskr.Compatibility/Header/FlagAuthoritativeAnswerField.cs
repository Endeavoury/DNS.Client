namespace DNS.Client;

/// <summary>
/// A one bit field that specifies whether the responding name server is an authority.
/// </summary>
public class FlagAuthoritativeAnswerField
{
    private FlagAuthoritativeAnswer value;

    public FlagAuthoritativeAnswerField(FlagAuthoritativeAnswer value)
    {
        this.value= value;
    }

    public byte GetByte()
    {
        return (byte)value;
    }
}
