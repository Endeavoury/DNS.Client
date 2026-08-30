using System;

namespace DNS.Client;

/// <summary>
/// an unsigned 16 bit integer specifying the number of
/// resource records in the answer section.
/// </summary>
public class AnCount
{
    public ushort value;

    public AnCount(ushort value)
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
