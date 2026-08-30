using System;
using System.Collections.Generic;

namespace DNS.Client;

public class QuestionField
{
    public QuestionField(Question value)
    {
        Domain = value.Domain;
        Type = value.Type;
        Class = value.Class;
    }

    public string Domain { get; private set; }
    public QuestionType Type { get; private set; }
    public QuestionClass Class { get; private set; }

    public string[] DomainSections => Domain.Split('.');

    public byte[] GetBytes()
    {
        return DnsMessage.CreateQuery(0, Domain, Type, Class).ToArray()[12..];
    }
}
