namespace DNS.Client;

/// <summary>
///  A four bit field that specifies kind of query in this
///  message.This value is set by the originator of a query
///  and copied into the response.    
/// </summary>
public enum FlagOpcode : byte
{
    /// <summary>
    /// a standard query (QUERY)
    /// </summary>
    /// 
    Query = 0,
    /// <summary>
    /// an inverse query (IQUERY)
    /// </summary>
    Iquery = 1 << 0,

    /// <summary>
    /// reserved for future use
    /// </summary>
    Status = 1 << 1
}
