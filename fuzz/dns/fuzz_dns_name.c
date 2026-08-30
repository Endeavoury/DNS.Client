#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "protocols/dns/dns_internal.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    uint8_t packet[1024] = {0x12,0x34,0x81,0x80,0,1,0,0,0,0,0,0};
    size_t copied = size < sizeof(packet) - 16u ? size : sizeof(packet) - 16u;
    ratos_context *ctx; ratos_dns_result *result = NULL;
    memcpy(packet + 12u, data, copied);
    packet[12u + copied] = 0u; packet[13u + copied] = 0u; packet[14u + copied] = 1u;
    packet[15u + copied] = 0u; packet[16u + copied] = 1u;
    ctx = ratos_context_create(); if (ctx == NULL) return 0;
    (void)ratos_dns_parse_response(ctx, packet, copied + 17u, 0x1234u, "example.com", RATOS_DNS_A, "fuzzer", &result);
    ratos_dns_result_destroy(result); ratos_context_destroy(ctx); return 0;
}
