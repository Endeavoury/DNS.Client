#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "protocols/dns/dns_internal.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    uint8_t packet[65535] = {0x12,0x34,0x81,0x80,0,1,0,1,0,0,0,0,
        7,'e','x','a','m','p','l','e',3,'c','o','m',0,0,1,0,1,0xc0,0x0c};
    const size_t fixed = 31u;
    size_t payload = size > 2u ? size - 2u : 0u;
    size_t copied = payload < sizeof(packet) - fixed - 10u ? payload : sizeof(packet) - fixed - 10u;
    size_t p = fixed;
    ratos_context *ctx; ratos_dns_result *result = NULL;
    packet[p++]=size > 0u ? data[0] : 0u; packet[p++]=size > 1u ? data[1] : 1u;
    packet[p++]=0; packet[p++]=1;
    packet[p++]=0; packet[p++]=0; packet[p++]=0; packet[p++]=1;
    packet[p++]=(uint8_t)(copied >> 8); packet[p++]=(uint8_t)copied;
    if (copied != 0u) memcpy(packet + p, data + 2u, copied);
    p += copied;
    ctx = ratos_context_create(); if (ctx == NULL) return 0;
    (void)ratos_dns_parse_response(ctx, packet, p, 0x1234u, "example.com", RATOS_DNS_A, "fuzzer", &result);
    ratos_dns_result_destroy(result); ratos_context_destroy(ctx); return 0;
}
