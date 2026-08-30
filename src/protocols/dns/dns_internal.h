#ifndef RATOS_DNS_INTERNAL_H
#define RATOS_DNS_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include "core/core_internal.h"
#include "ratatoskr/dns.h"

#define RATOS_DNS_MAX_PACKET 65535u
#define RATOS_DNS_MAX_RECORDS 4096u
#define RATOS_DNS_HEADER_SIZE 12u

struct ratos_dns_record {
    uint16_t type;
    ratos_dns_section section;
    char *name;
    uint32_t ttl;
    uint8_t *raw_data;
    size_t raw_length;
    char *text;
    uint16_t values16[3];
    size_t values16_count;
    uint32_t values32[5];
    size_t values32_count;
    char **strings;
    size_t string_count;
};

struct ratos_dns_result {
    uint16_t transaction_id;
    uint8_t rcode;
    uint8_t authoritative;
    uint8_t truncated;
    uint8_t recursion_desired;
    uint8_t recursion_available;
    uint8_t authentic_data;
    uint8_t checking_disabled;
    char *server;
    char *query_name;
    ratos_dns_type query_type;
    struct ratos_dns_record *records;
    size_t count;
};

typedef struct ratos_dns_packet {
    uint8_t *data;
    size_t length;
} ratos_dns_packet;

ratos_error ratos_dns_build_query(ratos_context *ctx, const char *name,
    ratos_dns_type type, uint8_t recursion_desired, uint16_t id,
    ratos_dns_packet *packet, char **effective_name);
ratos_error ratos_dns_parse_response(ratos_context *ctx, const uint8_t *data,
    size_t length, uint16_t expected_id, const char *expected_name,
    ratos_dns_type expected_type, const char *server, ratos_dns_result **out_result);

ratos_error ratos_dns_udp_exchange(ratos_context *ctx, const char *server,
    uint16_t port, uint32_t timeout_ms, const uint8_t *query, size_t query_length,
    uint8_t **response, size_t *response_length);
ratos_error ratos_dns_tcp_exchange(ratos_context *ctx, const char *server,
    uint16_t port, uint32_t timeout_ms, const uint8_t *query, size_t query_length,
    uint8_t **response, size_t *response_length);

void ratos_dns_record_clear(struct ratos_dns_record *record);
char *ratos_dns_default_server(void);

#endif
