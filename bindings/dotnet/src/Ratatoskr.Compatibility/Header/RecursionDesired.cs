namespace DNS.Client;

public enum RecursionDesired : byte
{
    NotDesired = 0,
    Desired = 1 << 0,
}
