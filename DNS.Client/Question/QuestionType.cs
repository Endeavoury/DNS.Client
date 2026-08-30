namespace DNS.Client;

public enum QuestionType : ushort
{
    /// <summary>
    /// 1 a host address
    /// </summary>
    /// 
    A = 1,
    /// <summary>
    /// 2 an authoritative name server
    /// </summary>
    NS = 2,

    /// <summary>
    /// 3 a mail destination (Obsolete - use MX)
    /// </summary>
    MD = 3,

    /// <summary>
    /// 4 a mail forwarder (Obsolete - use MX)
    /// </summary>
    MF = 4,

    /// <summary>
    /// 5 the canonical name for an alias
    /// </summary>
    CNAME = 5,

    /// <summary>
    /// 6 marks the start of a zone of authority
    /// </summary>
    SOA = 6,

    /// <summary>
    /// 7 a mailbox domain name (EXPERIMENTAL)
    /// </summary>
    MB = 7,

    /// <summary>
    /// 8 a mail group member (EXPERIMENTAL)
    /// </summary>
    MG = 8,

    /// <summary>
    /// 9 a mail rename domain name (EXPERIMENTAL)
    /// </summary>
    MR = 9,

    /// <summary>
    /// 10 a null RR (EXPERIMENTAL)
    /// </summary>
    NULL = 10,

    /// <summary>
    /// 11 a well known service description
    /// </summary>
    WKS = 11,

    /// <summary>
    ///  12 a domain name pointer
    /// </summary>
    PTR = 12,

    /// <summary>
    /// 13 host information
    /// </summary>
    HINFO = 13,

    /// <summary>
    /// 14 mailbox or mail list information
    /// </summary>
    MINFO = 14,

    /// <summary>
    /// 15 mail exchange
    /// </summary>
    MX = 15,

    /// <summary>
    /// 16 text strings
    /// </summary>
    TXT = 16,
    AXFR = 252,
    MAILB = 253,
    MAILA = 254,
    ANY = 255
}
