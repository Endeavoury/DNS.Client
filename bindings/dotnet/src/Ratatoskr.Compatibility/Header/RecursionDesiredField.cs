namespace DNS.Client;

public class RecursionDesiredField
{
    private RecursionDesired value;

    public RecursionDesiredField(RecursionDesired value)
    {
        this.value = value;
    }

    public byte GetByte()
    {
        return (byte)value;
    }
}
