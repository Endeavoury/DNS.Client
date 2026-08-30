#include <stdlib.h>
#include "protocols/dns/dns_internal.h"

void ratos_dns_record_clear(struct ratos_dns_record *record) {
    size_t i;
    if (record == NULL) return;
    free(record->name);
    free(record->raw_data);
    free(record->text);
    for (i = 0u; i < record->string_count; ++i) free(record->strings[i]);
    free(record->strings);
}

void ratos_dns_result_destroy(ratos_dns_result *result) {
    size_t i;
    if (result == NULL) return;
    if (result->records != NULL)
        for (i = 0; i < result->count; ++i) ratos_dns_record_clear(&result->records[i]);
    free(result->records);
    free(result->server);
    free(result->query_name);
    free(result);
}

uint8_t ratos_dns_result_rcode(const ratos_dns_result *r) { return r != NULL ? r->rcode : 0u; }
uint16_t ratos_dns_result_transaction_id(const ratos_dns_result *r) { return r != NULL ? r->transaction_id : 0u; }
uint8_t ratos_dns_result_authoritative(const ratos_dns_result *r) { return r != NULL ? r->authoritative : 0u; }
uint8_t ratos_dns_result_truncated(const ratos_dns_result *r) { return r != NULL ? r->truncated : 0u; }
uint8_t ratos_dns_result_recursion_desired(const ratos_dns_result *r) { return r != NULL ? r->recursion_desired : 0u; }
uint8_t ratos_dns_result_recursion_available(const ratos_dns_result *r) { return r != NULL ? r->recursion_available : 0u; }
uint8_t ratos_dns_result_authentic_data(const ratos_dns_result *r) { return r != NULL ? r->authentic_data : 0u; }
uint8_t ratos_dns_result_checking_disabled(const ratos_dns_result *r) { return r != NULL ? r->checking_disabled : 0u; }
const char *ratos_dns_result_server(const ratos_dns_result *r) { return r != NULL ? r->server : NULL; }
const char *ratos_dns_result_query_name(const ratos_dns_result *r) { return r != NULL ? r->query_name : NULL; }
ratos_dns_type ratos_dns_result_query_type(const ratos_dns_result *r) { return r != NULL ? r->query_type : (ratos_dns_type)0; }
size_t ratos_dns_result_count(const ratos_dns_result *r) { return r != NULL ? r->count : 0u; }
const ratos_dns_record *ratos_dns_result_record(const ratos_dns_result *r, size_t i) {
    return r != NULL && i < r->count ? &r->records[i] : NULL;
}
ratos_dns_type ratos_dns_record_type(const ratos_dns_record *r) { return r != NULL ? (ratos_dns_type)r->type : (ratos_dns_type)0; }
uint16_t ratos_dns_record_type_code(const ratos_dns_record *r) { return r != NULL ? r->type : 0u; }
ratos_dns_section ratos_dns_record_section(const ratos_dns_record *r) { return r != NULL ? r->section : (ratos_dns_section)0; }
const char *ratos_dns_record_name(const ratos_dns_record *r) { return r != NULL ? r->name : NULL; }
uint32_t ratos_dns_record_ttl(const ratos_dns_record *r) { return r != NULL ? r->ttl : 0u; }
const uint8_t *ratos_dns_record_raw_data(const ratos_dns_record *r, size_t *length) {
    if (length != NULL) *length = r != NULL ? r->raw_length : 0u;
    return r != NULL ? r->raw_data : NULL;
}
const char *ratos_dns_record_text(const ratos_dns_record *r) { return r != NULL ? r->text : NULL; }
int ratos_dns_record_uint16(const ratos_dns_record *r, size_t index, uint16_t *value) {
    if (r == NULL || value == NULL || index >= r->values16_count) return 0;
    *value = r->values16[index]; return 1;
}
int ratos_dns_record_uint32(const ratos_dns_record *r, size_t index, uint32_t *value) {
    if (r == NULL || value == NULL || index >= r->values32_count) return 0;
    *value = r->values32[index]; return 1;
}
size_t ratos_dns_record_string_count(const ratos_dns_record *r) { return r != NULL ? r->string_count : 0u; }
const char *ratos_dns_record_string(const ratos_dns_record *r, size_t index) {
    return r != NULL && index < r->string_count ? r->strings[index] : NULL;
}

const char *ratos_dns_type_string(uint16_t type) {
    switch (type) {
    case RATOS_DNS_A: return "A"; case RATOS_DNS_NS: return "NS";
    case RATOS_DNS_MD: return "MD"; case RATOS_DNS_MF: return "MF";
    case RATOS_DNS_CNAME: return "CNAME"; case RATOS_DNS_SOA: return "SOA";
    case RATOS_DNS_MB: return "MB"; case RATOS_DNS_MG: return "MG";
    case RATOS_DNS_MR: return "MR"; case RATOS_DNS_NULL: return "NULL";
    case RATOS_DNS_WKS: return "WKS"; case RATOS_DNS_HINFO: return "HINFO";
    case RATOS_DNS_MINFO: return "MINFO";
    case RATOS_DNS_PTR: return "PTR"; case RATOS_DNS_MX: return "MX";
    case RATOS_DNS_TXT: return "TXT"; case RATOS_DNS_AAAA: return "AAAA";
    case RATOS_DNS_SRV: return "SRV"; case RATOS_DNS_NAPTR: return "NAPTR";
    case RATOS_DNS_CAA: return "CAA"; default: return "UNKNOWN";
    }
}

const char *ratos_dns_rcode_string(uint8_t rcode) {
    switch (rcode) {
    case 0: return "NOERROR"; case 1: return "FORMERR"; case 2: return "SERVFAIL";
    case 3: return "NXDOMAIN"; case 4: return "NOTIMP"; case 5: return "REFUSED";
    default: return "UNKNOWN";
    }
}
