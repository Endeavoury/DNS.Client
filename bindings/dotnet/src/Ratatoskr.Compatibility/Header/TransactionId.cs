using System;

namespace DNS.Client;

/// <summary>
/// A 16 bit identifier assigned by the program that
/// generates any kind of query.This identifier is copied
/// the corresponding reply and can be used by the requester
/// to match up replies to outstanding queries.
/// </summary>
public class TransactionId
{
    public ushort value;

    public TransactionId(ushort value)
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
