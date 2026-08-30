using System.Net;
using System.Net.Sockets;

namespace DNS.Client.Tests;

[TestClass]
public class TransportTests
{
    [TestMethod]
    public async Task QueriesOverUdp()
    {
        using var server = new UdpClient(new IPEndPoint(IPAddress.Loopback, 0));
        int port = ((IPEndPoint)server.Client.LocalEndPoint!).Port;
        Task serverTask = Task.Run(async () =>
        {
            UdpReceiveResult request = await server.ReceiveAsync();
            DnsMessage query = DnsMessage.Parse(request.Buffer);
            DnsMessage response = CreateResponse(query);
            byte[] bytes = response.ToArray();
            await server.SendAsync(bytes, bytes.Length, request.RemoteEndPoint);
        });
        var client = new DnsClient(IPAddress.Loopback, port) { Timeout = TimeSpan.FromSeconds(2) };

        DnsMessage result = await client.QueryAsync("example.com");
        await serverTask;

        Assert.AreEqual(IPAddress.Parse("192.0.2.10"), ((ARecordData)result.Answers[0].Data).Address);
    }

    [TestMethod]
    public async Task TruncatedUdpResponseFallsBackToTcp()
    {
        var tcpServer = new TcpListener(IPAddress.Loopback, 0);
        tcpServer.Start();
        int port = ((IPEndPoint)tcpServer.LocalEndpoint).Port;
        using var udpServer = new UdpClient(new IPEndPoint(IPAddress.Loopback, port));

        Task udpTask = Task.Run(async () =>
        {
            UdpReceiveResult request = await udpServer.ReceiveAsync();
            DnsMessage query = DnsMessage.Parse(request.Buffer);
            var truncated = new DnsMessage(new DnsHeader
            {
                Id = query.Header.Id,
                IsResponse = true,
                IsTruncated = true,
                RecursionDesired = query.Header.RecursionDesired
            });
            truncated.Questions.Add(query.Questions[0]);
            byte[] bytes = truncated.ToArray();
            await udpServer.SendAsync(bytes, bytes.Length, request.RemoteEndPoint);
        });
        Task tcpTask = Task.Run(async () =>
        {
            using TcpClient connection = await tcpServer.AcceptTcpClientAsync();
            using NetworkStream stream = connection.GetStream();
            byte[] prefix = await ReadExactlyAsync(stream, 2);
            byte[] request = await ReadExactlyAsync(stream, (prefix[0] << 8) | prefix[1]);
            DnsMessage response = CreateResponse(DnsMessage.Parse(request));
            byte[] responseBytes = response.ToArray();
            byte[] responsePrefix = { (byte)(responseBytes.Length >> 8), (byte)responseBytes.Length };
            await stream.WriteAsync(responsePrefix, 0, responsePrefix.Length);
            await stream.WriteAsync(responseBytes, 0, responseBytes.Length);
        });

        try
        {
            var client = new DnsClient(IPAddress.Loopback, port) { Timeout = TimeSpan.FromSeconds(2) };
            DnsMessage result = await client.QueryAsync("example.com");
            await Task.WhenAll(udpTask, tcpTask);
            Assert.IsFalse(result.Header.IsTruncated);
            Assert.AreEqual(1, result.Answers.Count);
        }
        finally
        {
            tcpServer.Stop();
        }
    }

    private static DnsMessage CreateResponse(DnsMessage query)
    {
        var response = new DnsMessage(new DnsHeader
        {
            Id = query.Header.Id,
            IsResponse = true,
            OpCode = query.Header.OpCode,
            RecursionDesired = query.Header.RecursionDesired,
            RecursionAvailable = true
        });
        foreach (DnsQuestion question in query.Questions) response.Questions.Add(question);
        response.Answers.Add(new DnsResourceRecord(query.Questions[0].Name, QuestionType.A,
            QuestionClass.IN, 60, new ARecordData(IPAddress.Parse("192.0.2.10"))));
        return response;
    }

    private static async Task<byte[]> ReadExactlyAsync(NetworkStream stream, int length)
    {
        byte[] data = new byte[length];
        int offset = 0;
        while (offset < length)
        {
            int count = await stream.ReadAsync(data, offset, length - offset);
            if (count == 0) throw new IOException("Unexpected end of stream.");
            offset += count;
        }
        return data;
    }
}
