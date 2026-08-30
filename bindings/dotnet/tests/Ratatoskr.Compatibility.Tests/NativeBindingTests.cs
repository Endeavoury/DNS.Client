using System.Net;

namespace DNS.Client.Tests;

[TestClass]
public class NativeBindingTests
{
    [TestMethod]
    public void LoadsExpectedNativeAbi()
    {
        Assert.AreEqual(1u, Ratatoskr.Dns.NativeAbiVersion);
    }

    [TestMethod]
    public void CreatesIpv6ReverseName()
    {
        string name = DnsClient.GetReverseLookupName(IPAddress.Parse("2001:db8::1"));
        Assert.IsTrue(name.EndsWith(".ip6.arpa", StringComparison.Ordinal));
        Assert.IsTrue(name.StartsWith("1.0.0.0", StringComparison.Ordinal));
    }
}
