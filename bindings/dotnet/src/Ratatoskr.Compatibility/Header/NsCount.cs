using System;

namespace DNS.Client;

/// <summary>
/// an unsigned 16 bit integer specifying the number of
/// resource records in the additional records section.
/// </summary>
public class ArCount
{
    public ushort value;

    public ArCount(ushort value)
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
