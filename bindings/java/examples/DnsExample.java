import io.github.endeavoury.ratatoskr.DnsRecordType;
import io.github.endeavoury.ratatoskr.DnsResult;
import io.github.endeavoury.ratatoskr.Ratatoskr;

public final class DnsExample {
    private DnsExample() {}

    public static void main(String[] args) {
        DnsResult result = Ratatoskr.dns().query("example.com", DnsRecordType.A);
        result.answers().forEach(record -> System.out.println(record.text()));
    }
}
