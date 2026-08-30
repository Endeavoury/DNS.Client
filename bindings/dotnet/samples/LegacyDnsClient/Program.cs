namespace DNS.Client.Console;

public static class Program
{
    public static async Task Main(string[] args)
    {
        string name = args.Length > 0 ? args[0] : "example.com";
        var client = new DnsClient();
        DnsMessage response = await client.QueryAsync(name, QuestionType.A);

        foreach (DnsResourceRecord answer in response.Answers)
        {
            if (answer.Data is ARecordData address)
                System.Console.WriteLine($"{answer.Name} {answer.TimeToLive} IN A {address.Address}");
        }
    }
}
