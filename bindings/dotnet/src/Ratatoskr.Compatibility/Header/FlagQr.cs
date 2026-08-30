namespace DNS.Client;

/// <summary>
/// A one bit field that specifies whether this message is a query(0), or a response(1).
/// </summary>
public enum FlagQr : byte
{
    Query = 0,
    Response = 1 << 0
}
