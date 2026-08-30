using System;

namespace DNS.Client;

/// <summary>
/// an unsigned 16 bit integer specifying the number of
/// entries in the question section.
/// </summary>
public class QdCount
{
    public ushort value;

    public QdCount(ushort value)
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
