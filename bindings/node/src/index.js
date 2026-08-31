import koffi from "koffi";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const types = Object.freeze({ A: 1, NS: 2, CNAME: 5, SOA: 6, PTR: 12, MX: 15, TXT: 16, AAAA: 28, SRV: 33, NAPTR: 35, CAA: 257 });
let api;

function libraryPath() {
  if (process.env.RATATOSKR_LIBRARY) return process.env.RATATOSKR_LIBRARY;
  const base = dirname(fileURLToPath(import.meta.url));
  const filename = process.platform === "win32" ? "ratatoskr.dll" : process.platform === "darwin" ? "libratatoskr.dylib" : "libratatoskr.so";
  const bundled = join(base, "..", "native", `${process.platform}-${process.arch}`, filename);
  return existsSync(bundled) ? bundled : filename;
}

function native() {
  if (api) return api;
  const lib = koffi.load(libraryPath());
  const options = koffi.struct("ratos_dns_query_options", { struct_size: "uint32_t", server: "const char *", port: "uint16_t", type: "uint16_t", timeout_ms: "uint32_t", recursion_desired: "uint8_t", reserved: koffi.array("uint8_t", 7) });
  const resultPtr = koffi.pointer("ratos_dns_result", koffi.opaque());
  const recordPtr = koffi.pointer("ratos_dns_record", koffi.opaque());
  const contextPtr = koffi.pointer("ratos_context", koffi.opaque());
  api = {
    abi: lib.func("uint32_t ratos_abi_version(void)"),
    contextCreate: lib.func("ratos_context_create", contextPtr, []),
    contextDestroy: lib.func("ratos_context_destroy", "void", [contextPtr]),
    contextError: lib.func("ratos_context_error", "str", [contextPtr]),
    errorString: lib.func("ratos_error_string", "str", ["int"]),
    optionsInit: lib.func("ratos_dns_query_options_init", "void", [koffi.pointer(options)]),
    query: lib.func("ratos_dns_query", "int", [contextPtr, "str", koffi.pointer(options), koffi.out(koffi.pointer(resultPtr))]),
    destroy: lib.func("ratos_dns_result_destroy", "void", [resultPtr]),
    rcode: lib.func("ratos_dns_result_rcode", "uint8_t", [resultPtr]),
    queryName: lib.func("ratos_dns_result_query_name", "str", [resultPtr]),
    queryType: lib.func("ratos_dns_result_query_type", "uint16_t", [resultPtr]),
    server: lib.func("ratos_dns_result_server", "str", [resultPtr]),
    count: lib.func("ratos_dns_result_count", "size_t", [resultPtr]),
    record: lib.func("ratos_dns_result_record", recordPtr, [resultPtr, "size_t"]),
    recordType: lib.func("ratos_dns_record_type_code", "uint16_t", [recordPtr]),
    recordSection: lib.func("ratos_dns_record_section", "uint8_t", [recordPtr]),
    recordName: lib.func("ratos_dns_record_name", "str", [recordPtr]),
    recordTtl: lib.func("ratos_dns_record_ttl", "uint32_t", [recordPtr]),
    recordText: lib.func("ratos_dns_record_text", "str", [recordPtr])
  };
  return api;
}

export function abiVersion() { return native().abi(); }

export function querySync(name, options = {}) {
  if (typeof name !== "string" || !name || name.includes("\0")) throw new TypeError("name must be a non-empty string without NUL");
  const binding = native();
  if (binding.abi() !== 1) throw new Error("unsupported Ratatoskr native ABI");
  const requestedType = typeof options.type === "number" ? options.type : types[String(options.type ?? "A").toUpperCase()];
  if (!requestedType) throw new TypeError(`unsupported DNS record type: ${options.type}`);
  const context = binding.contextCreate();
  if (!context) throw new Error("could not allocate native context");
  const nativeOptions = {};
  const output = [null];
  try {
    binding.optionsInit(nativeOptions);
    nativeOptions.server = options.server ?? null;
    nativeOptions.port = options.port ?? 53;
    nativeOptions.type = requestedType;
    nativeOptions.timeout_ms = options.timeoutMs ?? 5000;
    nativeOptions.recursion_desired = options.recursionDesired === false ? 0 : 1;
    const code = binding.query(context, name, nativeOptions, output);
    if (code !== 0) throw Object.assign(new Error(binding.contextError(context) || binding.errorString(code)), { code });
    const result = output[0];
    if (!result) throw new Error("native query returned no result");
    try {
      const count = Number(binding.count(result));
      if (count > 4096) throw new Error("native result exceeds safety limit");
      const records = [];
      for (let index = 0; index < count; index++) {
        const record = binding.record(result, index);
        records.push({ typeCode: binding.recordType(record), section: binding.recordSection(record), name: binding.recordName(record) ?? "", ttl: binding.recordTtl(record), text: binding.recordText(record) ?? "" });
      }
      return { queryName: binding.queryName(result) ?? "", queryType: binding.queryType(result), server: binding.server(result) ?? "", responseCode: binding.rcode(result), records };
    } finally { binding.destroy(result); }
  } finally { binding.contextDestroy(context); }
}

export const dns = Object.freeze({ types, querySync, query: async (name, options = {}) => querySync(name, options) });
