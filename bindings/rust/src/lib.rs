//! Safe, ownership-preserving bindings to the canonical Ratatoskr C core.

use std::ffi::{CStr, CString};
use std::fmt;
use std::os::raw::{c_char, c_int, c_uchar, c_uint, c_ushort, c_void};
use std::ptr;

const ABI_VERSION: u32 = 1;

#[repr(C)]
struct NativeOptions {
    struct_size: c_uint,
    server: *const c_char,
    port: c_ushort,
    record_type: c_ushort,
    timeout_ms: c_uint,
    recursion_desired: c_uchar,
    reserved: [c_uchar; 7],
}

#[link(name = "ratatoskr")]
extern "C" {
    fn ratos_abi_version() -> c_uint;
    fn ratos_context_create() -> *mut c_void;
    fn ratos_context_destroy(context: *mut c_void);
    fn ratos_context_error(context: *const c_void) -> *const c_char;
    fn ratos_error_string(error: c_int) -> *const c_char;
    fn ratos_dns_query_options_init(options: *mut NativeOptions);
    fn ratos_dns_query(context: *mut c_void, name: *const c_char, options: *const NativeOptions, result: *mut *mut c_void) -> c_int;
    fn ratos_dns_result_destroy(result: *mut c_void);
    fn ratos_dns_result_rcode(result: *const c_void) -> c_uchar;
    fn ratos_dns_result_query_name(result: *const c_void) -> *const c_char;
    fn ratos_dns_result_query_type(result: *const c_void) -> c_ushort;
    fn ratos_dns_result_server(result: *const c_void) -> *const c_char;
    fn ratos_dns_result_count(result: *const c_void) -> usize;
    fn ratos_dns_result_record(result: *const c_void, index: usize) -> *const c_void;
    fn ratos_dns_record_type_code(record: *const c_void) -> c_ushort;
    fn ratos_dns_record_section(record: *const c_void) -> c_uchar;
    fn ratos_dns_record_name(record: *const c_void) -> *const c_char;
    fn ratos_dns_record_ttl(record: *const c_void) -> c_uint;
    fn ratos_dns_record_text(record: *const c_void) -> *const c_char;
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u16)]
pub enum RecordType { A = 1, Ns = 2, Cname = 5, Soa = 6, Ptr = 12, Mx = 15, Txt = 16, Aaaa = 28, Srv = 33, Naptr = 35, Caa = 257 }

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct QueryOptions { pub server: Option<String>, pub port: u16, pub timeout_ms: u32, pub recursion_desired: bool }

impl Default for QueryOptions {
    fn default() -> Self { Self { server: None, port: 53, timeout_ms: 5_000, recursion_desired: true } }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Record { pub type_code: u16, pub section: u8, pub name: String, pub ttl: u32, pub text: String }

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct DnsResult { pub query_name: String, pub query_type: u16, pub server: String, pub response_code: u8, pub records: Vec<Record> }

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Error { pub code: i32, pub message: String }

impl fmt::Display for Error { fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "Ratatoskr error {}: {}", self.code, self.message) } }
impl std::error::Error for Error {}

unsafe fn text(pointer: *const c_char) -> String {
    if pointer.is_null() { String::new() } else { CStr::from_ptr(pointer).to_string_lossy().into_owned() }
}

pub fn abi_version() -> u32 { unsafe { ratos_abi_version() } }

pub fn query(name: &str, record_type: RecordType, options: &QueryOptions) -> std::result::Result<DnsResult, Error> {
    if abi_version() != ABI_VERSION { return Err(Error { code: 9, message: "unsupported native ABI".into() }); }
    let name = CString::new(name).map_err(|_| Error { code: 2, message: "name contains NUL".into() })?;
    let server = options.server.as_ref().map(|s| CString::new(s.as_str())).transpose().map_err(|_| Error { code: 2, message: "server contains NUL".into() })?;
    unsafe {
        let context = ratos_context_create();
        if context.is_null() { return Err(Error { code: 3, message: "could not allocate native context".into() }); }
        let mut native: NativeOptions = std::mem::zeroed();
        ratos_dns_query_options_init(&mut native);
        native.server = server.as_ref().map_or(ptr::null(), |s| s.as_ptr());
        native.port = options.port;
        native.record_type = record_type as u16;
        native.timeout_ms = options.timeout_ms;
        native.recursion_desired = options.recursion_desired as u8;
        let mut result = ptr::null_mut();
        let code = ratos_dns_query(context, name.as_ptr(), &native, &mut result);
        if code != 0 {
            let detail = text(ratos_context_error(context));
            let message = if detail.is_empty() { text(ratos_error_string(code)) } else { detail };
            ratos_context_destroy(context);
            return Err(Error { code, message });
        }
        let count = ratos_dns_result_count(result);
        if count > 4096 { ratos_dns_result_destroy(result); ratos_context_destroy(context); return Err(Error { code: 6, message: "native result exceeds safety limit".into() }); }
        let mut records = Vec::with_capacity(count);
        for index in 0..count {
            let record = ratos_dns_result_record(result, index);
            if record.is_null() { continue; }
            records.push(Record { type_code: ratos_dns_record_type_code(record), section: ratos_dns_record_section(record), name: text(ratos_dns_record_name(record)), ttl: ratos_dns_record_ttl(record), text: text(ratos_dns_record_text(record)) });
        }
        let owned = DnsResult { query_name: text(ratos_dns_result_query_name(result)), query_type: ratos_dns_result_query_type(result), server: text(ratos_dns_result_server(result)), response_code: ratos_dns_result_rcode(result), records };
        ratos_dns_result_destroy(result);
        ratos_context_destroy(context);
        Ok(owned)
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn native_abi_is_supported() { assert_eq!(super::abi_version(), 1); }
    #[test]
    fn defaults_are_stable() { let value = super::QueryOptions::default(); assert_eq!((value.port, value.timeout_ms), (53, 5_000)); }
}
