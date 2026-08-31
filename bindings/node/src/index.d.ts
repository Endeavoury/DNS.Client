export type DnsRecordType = "A" | "NS" | "CNAME" | "SOA" | "PTR" | "MX" | "TXT" | "AAAA" | "SRV" | "NAPTR" | "CAA";
export interface DnsQueryOptions { type?: DnsRecordType | number; server?: string; port?: number; timeoutMs?: number; recursionDesired?: boolean; }
export interface DnsRecord { readonly typeCode: number; readonly section: number; readonly name: string; readonly ttl: number; readonly text: string; }
export interface DnsResult { readonly queryName: string; readonly queryType: number; readonly server: string; readonly responseCode: number; readonly records: readonly DnsRecord[]; }
export declare function abiVersion(): number;
export declare function querySync(name: string, options?: DnsQueryOptions): DnsResult;
export declare const dns: { readonly types: Readonly<Record<DnsRecordType, number>>; querySync: typeof querySync; query(name: string, options?: DnsQueryOptions): Promise<DnsResult>; };
