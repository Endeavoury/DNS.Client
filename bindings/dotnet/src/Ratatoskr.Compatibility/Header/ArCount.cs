using System;

namespace DNS.Client;

/// <summary>
/// an unsigned 16 bit integer specifying the number of name
/// server resource records in the authority records section.
/// </summary>
public class NsCount
{
    public ushort value;

    public NsCount(ushort value)
    {
        this.value = value;
    }

    public byte[] ToByteArray()
    {
        byte[] bytes = BitConverter.GetBytes(value);
        Array.Reverse(bytes, 0, bytes.Length);
        return bytes;
    }
}
