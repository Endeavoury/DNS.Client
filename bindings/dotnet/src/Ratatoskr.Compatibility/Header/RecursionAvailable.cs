namespace DNS.Client;

public enum RecursionAvailable : byte
{
    NotAvailable = 0,
    Available = 1 << 0,
}
