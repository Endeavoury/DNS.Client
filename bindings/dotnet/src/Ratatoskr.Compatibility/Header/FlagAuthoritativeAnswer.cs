namespace DNS.Client;

/// <summary>
/// Authoritative Answer - 
/// this bit is valid in responses, and specifies that the responding name server is an    
/// authority for the domain name in question section.
/// Note that the contents of the answer section may have multiple owner names because of aliases.The AA bit corresponds to the name which matches the query name, or
/// the first owner name in the answer section.
/// </summary>
public enum FlagAuthoritativeAnswer : byte
{
    NotTheOwner = 0,
    Owner  = 1 << 0
}
