import CRatatoskr

public enum DnsRecordType: UInt16, Sendable {
    case a = 1, ns = 2, cname = 5, soa = 6, ptr = 12, mx = 15, txt = 16
    case aaaa = 28, srv = 33, naptr = 35, caa = 257
}

public struct DnsQueryOptions: Sendable {
    public var server: String?
    public var port: UInt16
    public var timeoutMilliseconds: UInt32
    public var recursionDesired: Bool
    public init(server: String? = nil, port: UInt16 = 53, timeoutMilliseconds: UInt32 = 5_000, recursionDesired: Bool = true) {
        self.server = server; self.port = port; self.timeoutMilliseconds = timeoutMilliseconds; self.recursionDesired = recursionDesired
    }
}

public struct DnsRecord: Sendable { public let typeCode: UInt16; public let section: UInt8; public let name: String; public let ttl: UInt32; public let text: String }
public struct DnsResult: Sendable { public let queryName: String; public let queryType: UInt16; public let server: String; public let responseCode: UInt8; public let records: [DnsRecord] }
public struct RatatoskrError: Error, Sendable { public let code: Int32; public let message: String }

public enum Dns {
    public static var abiVersion: UInt32 { ratos_abi_version() }

    public static func query(_ name: String, type: DnsRecordType = .a, options: DnsQueryOptions = .init()) throws -> DnsResult {
        guard abiVersion == RATOS_ABI_VERSION else { throw RatatoskrError(code: 9, message: "unsupported native ABI") }
        guard !name.isEmpty && !name.utf8.contains(0) else { throw RatatoskrError(code: 2, message: "invalid query name") }
        guard let context = ratos_context_create() else { throw RatatoskrError(code: 3, message: "could not allocate native context") }
        defer { ratos_context_destroy(context) }
        var nativeOptions = ratos_dns_query_options()
        ratos_dns_query_options_init(&nativeOptions)
        nativeOptions.port = options.port
        nativeOptions.type = type.rawValue
        nativeOptions.timeout_ms = options.timeoutMilliseconds
        nativeOptions.recursion_desired = options.recursionDesired ? 1 : 0
        var nativeResult: OpaquePointer?
        let code: ratos_error = name.withCString { queryName in
            if let server = options.server {
                return server.withCString { serverName in
                    nativeOptions.server = serverName
                    return ratos_dns_query(context, queryName, &nativeOptions, &nativeResult)
                }
            }
            nativeOptions.server = nil
            return ratos_dns_query(context, queryName, &nativeOptions, &nativeResult)
        }
        guard code == RATOS_OK, let result = nativeResult else {
            let detail = ratos_context_error(context).map(String.init(cString:)) ?? String(cString: ratos_error_string(code))
            throw RatatoskrError(code: Int32(code.rawValue), message: detail)
        }
        defer { ratos_dns_result_destroy(result) }
        let count = ratos_dns_result_count(result)
        guard count <= 4096 else { throw RatatoskrError(code: 6, message: "native result exceeds safety limit") }
        var records: [DnsRecord] = []
        records.reserveCapacity(count)
        for index in 0..<count {
            guard let item = ratos_dns_result_record(result, index) else { continue }
            records.append(DnsRecord(typeCode: ratos_dns_record_type_code(item), section: ratos_dns_record_section(item), name: string(ratos_dns_record_name(item)), ttl: ratos_dns_record_ttl(item), text: string(ratos_dns_record_text(item))))
        }
        return DnsResult(queryName: string(ratos_dns_result_query_name(result)), queryType: ratos_dns_result_query_type(result), server: string(ratos_dns_result_server(result)), responseCode: ratos_dns_result_rcode(result), records: records)
    }

    private static func string(_ value: UnsafePointer<CChar>?) -> String { value.map(String.init(cString:)) ?? "" }
}
