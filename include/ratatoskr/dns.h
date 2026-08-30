#ifndef RATATOSKR_DNS_H
#define RATATOSKR_DNS_H
#include <stddef.h>
#include <stdint.h>
#include "ratatoskr/context.h"
#include "ratatoskr/error.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ratos_dns_type;
enum {
    RATOS_DNS_A = 1, RATOS_DNS_NS = 2, RATOS_DNS_MD = 3, RATOS_DNS_MF = 4,
    RATOS_DNS_CNAME = 5, RATOS_DNS_SOA = 6, RATOS_DNS_MB = 7, RATOS_DNS_MG = 8,
    RATOS_DNS_MR = 9, RATOS_DNS_NULL = 10, RATOS_DNS_WKS = 11, RATOS_DNS_PTR = 12,
    RATOS_DNS_HINFO = 13, RATOS_DNS_MINFO = 14, RATOS_DNS_MX = 15, RATOS_DNS_TXT = 16,
    RATOS_DNS_AAAA = 28, RATOS_DNS_SRV = 33, RATOS_DNS_NAPTR = 35,
    RATOS_DNS_CAA = 257
};

typedef uint8_t ratos_dns_section;
enum {
    RATOS_DNS_SECTION_ANSWER = 1,
    RATOS_DNS_SECTION_AUTHORITY = 2,
    RATOS_DNS_SECTION_ADDITIONAL = 3
};

/* Initialize with ratos_dns_query_options_init before use. server is borrowed only
 * for the duration of ratos_dns_query. Zero port/timeout select defaults (53/5000). */
typedef struct ratos_dns_query_options {
    uint32_t struct_size;
    const char *server;
    uint16_t port;
    ratos_dns_type type;
    uint32_t timeout_ms;
    uint8_t recursion_desired;
    uint8_t reserved[7];
} ratos_dns_query_options;

typedef struct ratos_dns_result ratos_dns_result;
typedef struct ratos_dns_record ratos_dns_record;

RATOS_API void ratos_dns_query_options_init(ratos_dns_query_options *options);
/* On success, *out_result is owned by the caller. Destroy it with
 * ratos_dns_result_destroy. On error it is set to NULL. */
RATOS_API ratos_error ratos_dns_query(ratos_context *ctx, const char *name,
    const ratos_dns_query_options *options, ratos_dns_result **out_result);
RATOS_API void ratos_dns_result_destroy(ratos_dns_result *result);

RATOS_API uint8_t ratos_dns_result_rcode(const ratos_dns_result *result);
RATOS_API uint16_t ratos_dns_result_transaction_id(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_authoritative(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_truncated(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_recursion_desired(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_recursion_available(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_authentic_data(const ratos_dns_result *result);
RATOS_API uint8_t ratos_dns_result_checking_disabled(const ratos_dns_result *result);
RATOS_API const char *ratos_dns_result_server(const ratos_dns_result *result);
RATOS_API const char *ratos_dns_result_query_name(const ratos_dns_result *result);
RATOS_API ratos_dns_type ratos_dns_result_query_type(const ratos_dns_result *result);
RATOS_API size_t ratos_dns_result_count(const ratos_dns_result *result);
/* Returned record and string pointers are borrowed and live until result destruction. */
RATOS_API const ratos_dns_record *ratos_dns_result_record(const ratos_dns_result *result, size_t index);
RATOS_API ratos_dns_type ratos_dns_record_type(const ratos_dns_record *record);
RATOS_API uint16_t ratos_dns_record_type_code(const ratos_dns_record *record);
RATOS_API ratos_dns_section ratos_dns_record_section(const ratos_dns_record *record);
RATOS_API const char *ratos_dns_record_name(const ratos_dns_record *record);
RATOS_API uint32_t ratos_dns_record_ttl(const ratos_dns_record *record);
RATOS_API const uint8_t *ratos_dns_record_raw_data(const ratos_dns_record *record, size_t *length);
RATOS_API const char *ratos_dns_record_text(const ratos_dns_record *record);
/* Extensible typed fields. The meaning is determined by record type: MX has one
 * uint16 and one string; SRV has three uint16s and one string; SOA has two strings
 * and five uint32s; CAA has flags as uint16 plus tag/value strings; TXT has one
 * string per character-string. Returns 1 when the indexed field exists. */
RATOS_API int ratos_dns_record_uint16(const ratos_dns_record *record, size_t index, uint16_t *value);
RATOS_API int ratos_dns_record_uint32(const ratos_dns_record *record, size_t index, uint32_t *value);
RATOS_API size_t ratos_dns_record_string_count(const ratos_dns_record *record);
RATOS_API const char *ratos_dns_record_string(const ratos_dns_record *record, size_t index);
RATOS_API const char *ratos_dns_type_string(uint16_t type);
RATOS_API const char *ratos_dns_rcode_string(uint8_t rcode);

#ifdef __cplusplus
}
#endif
#endif
