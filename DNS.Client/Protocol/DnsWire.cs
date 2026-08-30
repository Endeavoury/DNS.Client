using System.Buffers.Binary;
using System.Text;

namespace DNS.Client;

internal sealed class DnsWireWriter
{
    private readonly List<byte> bytes = new();
    private readonly Dictionary<string, ushort> suffixOffsets = new(StringComparer.OrdinalIgnoreCase);
    private readonly bool compress;

    public DnsWireWriter(bool compress) => this.compress = compress;
    public int Position => bytes.Count;
    public byte[] ToArray() => bytes.ToArray();
    public void Byte(byte value) => bytes.Add(value);
    public void Bytes(ReadOnlySpan<byte> value) { foreach (byte b in value) bytes.Add(b); }
    public void UInt16(ushort value) { bytes.Add((byte)(value >> 8)); bytes.Add((byte)value); }
    public void UInt32(uint value)
    {
        bytes.Add((byte)(value >> 24)); bytes.Add((byte)(value >> 16));
        bytes.Add((byte)(value >> 8)); bytes.Add((byte)value);
    }

    public void PatchUInt16(int offset, ushort value)
    {
        bytes[offset] = (byte)(value >> 8);
        bytes[offset + 1] = (byte)value;
    }

    public void Name(string name)
    {
        if (name is null) throw new ArgumentNullException(nameof(name));
        if (name == "." || name.Length == 0) { Byte(0); return; }

        IReadOnlyList<byte[]> labels = ParseLabels(name);

        int wireLength = 1;
        foreach (byte[] label in labels)
        {
            if (label.Length > 63) throw new ArgumentException("A DNS label cannot exceed 63 octets.", nameof(name));
            wireLength += 1 + label.Length;
        }
        if (wireLength > 255) throw new ArgumentException("A DNS name cannot exceed 255 wire octets.", nameof(name));

        for (int index = 0; index < labels.Count; index++)
        {
            string suffix = string.Join(".", labels.Skip(index).Select(ToHexString));
            if (compress && suffixOffsets.TryGetValue(suffix, out ushort pointer))
            {
                UInt16((ushort)(0xc000 | pointer));
                return;
            }
            if (compress && Position < 0x4000) suffixOffsets.TryAdd(suffix, (ushort)Position);

            byte[] labelBytes = labels[index];
            Byte((byte)labelBytes.Length);
            Bytes(labelBytes);
        }
        Byte(0);
    }

    private static IReadOnlyList<byte[]> ParseLabels(string name)
    {
        var labels = new List<byte[]>();
        var label = new List<byte>();
        bool absolute = false;
        for (int index = 0; index < name.Length; index++)
        {
            char current = name[index];
            if (current == '.')
            {
                if (label.Count == 0) throw new ArgumentException("A domain name cannot contain an empty label.", nameof(name));
                labels.Add(label.ToArray());
                label.Clear();
                absolute = index == name.Length - 1;
                continue;
            }
            if (current == '\\')
            {
                if (++index >= name.Length) throw new ArgumentException("A domain-name escape is incomplete.", nameof(name));
                if (index + 2 < name.Length && IsAsciiDigit(name[index])
                    && IsAsciiDigit(name[index + 1]) && IsAsciiDigit(name[index + 2]))
                {
                    int value = (name[index] - '0') * 100 + (name[index + 1] - '0') * 10 + name[index + 2] - '0';
                    if (value > 255) throw new ArgumentException("A decimal domain-name escape cannot exceed 255.", nameof(name));
                    label.Add((byte)value);
                    index += 2;
                    continue;
                }
                current = name[index];
            }
            try
            {
                byte[] encoded = WireEncoding.GetBytes(new[] { current });
                label.Add(encoded[0]);
            }
            catch (EncoderFallbackException ex)
            {
                throw new ArgumentException("RFC 1035 names contain octets; encode international names before querying.", nameof(name), ex);
            }
        }
        if (!absolute)
        {
            if (label.Count == 0) throw new ArgumentException("A domain name cannot contain an empty label.", nameof(name));
            labels.Add(label.ToArray());
        }
        return labels;
    }

    private static bool IsAsciiDigit(char value) => value is >= '0' and <= '9';

    public void CharacterString(string value)
    {
        byte[] encoded = WireEncoding.GetBytes(value ?? throw new ArgumentNullException(nameof(value)));
        if (encoded.Length > 255) throw new ArgumentException("A DNS character-string cannot exceed 255 octets.", nameof(value));
        Byte((byte)encoded.Length);
        Bytes(encoded);
    }

    // RFC 1035 labels and character-strings are octet sequences, not Unicode text.
    // Latin-1 provides a lossless byte-to-char mapping; callers should encode IDNs explicitly.
    internal static readonly Encoding WireEncoding = Encoding.GetEncoding(
        28591, EncoderFallback.ExceptionFallback, DecoderFallback.ExceptionFallback);

    private static string ToHexString(byte[] value)
    {
        var builder = new StringBuilder(value.Length * 2);
        foreach (byte item in value) builder.Append(item.ToString("X2", System.Globalization.CultureInfo.InvariantCulture));
        return builder.ToString();
    }
}

internal ref struct DnsWireReader
{
    private readonly ReadOnlySpan<byte> data;
    public DnsWireReader(ReadOnlySpan<byte> data) { this.data = data; Position = 0; }
    public int Position { get; private set; }
    public int Length => data.Length;

    public byte Byte()
    {
        Ensure(1);
        return data[Position++];
    }

    public ushort UInt16()
    {
        Ensure(2);
        ushort value = BinaryPrimitives.ReadUInt16BigEndian(data[Position..]);
        Position += 2;
        return value;
    }

    public uint UInt32()
    {
        Ensure(4);
        uint value = BinaryPrimitives.ReadUInt32BigEndian(data[Position..]);
        Position += 4;
        return value;
    }

    public byte[] Bytes(int count)
    {
        Ensure(count);
        byte[] value = data.Slice(Position, count).ToArray();
        Position += count;
        return value;
    }

    public string Name()
    {
        var labels = new List<string>();
        int cursor = Position;
        int? resume = null;
        var visited = new HashSet<int>();
        int wireLength = 1;

        while (true)
        {
            if ((uint)cursor >= (uint)data.Length) throw Error("Domain name extends past the message.");
            byte length = data[cursor++];
            if ((length & 0xc0) == 0xc0)
            {
                if ((uint)cursor >= (uint)data.Length) throw Error("Incomplete compression pointer.");
                int pointerLocation = cursor - 1;
                int pointer = ((length & 0x3f) << 8) | data[cursor++];
                if (pointer >= data.Length) throw Error("Compression pointer is outside the message.");
                if (pointer >= pointerLocation) throw Error("Compression pointer does not reference a prior name.");
                resume ??= cursor;
                if (!visited.Add(pointer) || visited.Count > 128) throw Error("Compression pointer loop detected.");
                cursor = pointer;
                continue;
            }
            if ((length & 0xc0) != 0) throw Error("Unsupported DNS label encoding.");
            if (length == 0)
            {
                Position = resume ?? cursor;
                return labels.Count == 0 ? "." : string.Join('.', labels);
            }
            if (length > 63 || cursor + length > data.Length) throw Error("Invalid DNS label length.");
            wireLength += length + 1;
            if (wireLength > 255) throw Error("Domain name exceeds 255 wire octets.");
            labels.Add(EscapeLabel(data.Slice(cursor, length)));
            cursor += length;
        }
    }

    private static string EscapeLabel(ReadOnlySpan<byte> label)
    {
        var builder = new StringBuilder(label.Length);
        foreach (byte value in label)
        {
            if (value is >= 0x21 and <= 0x7e)
            {
                char character = (char)value;
                if (character is '.' or '\\') builder.Append('\\');
                builder.Append(character);
            }
            else
            {
                builder.Append('\\');
                builder.Append(value.ToString("D3", System.Globalization.CultureInfo.InvariantCulture));
            }
        }
        return builder.ToString();
    }

    public string CharacterString(int end)
    {
        int length = Byte();
        if (Position + length > end) throw Error("Character-string extends beyond RDLENGTH.");
        return DnsWireWriter.WireEncoding.GetString(Bytes(length));
    }

    public void Seek(int position)
    {
        if ((uint)position > (uint)data.Length) throw Error("Position is outside the message.");
        Position = position;
    }

    private void Ensure(int count)
    {
        if (count < 0 || Position > data.Length - count) throw Error("DNS message is truncated.");
    }

    private static DnsProtocolException Error(string message) => new(message);
}
