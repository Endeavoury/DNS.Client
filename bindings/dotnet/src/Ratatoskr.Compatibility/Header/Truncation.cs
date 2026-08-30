namespace DNS.Client;

public enum Truncation : byte
{
    Permitted = 0,
    Truncated = 1 << 0
}
