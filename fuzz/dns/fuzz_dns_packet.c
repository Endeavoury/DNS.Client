#include <stddef.h>
#include <stdint.h>
#include "protocols/dns/dns_internal.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    ratos_context *ctx;
    ratos_dns_result *result = NULL;
    if (size < 2u) return 0;
    ctx = ratos_context_create();
    if (ctx == NULL) return 0;
    (void)ratos_dns_parse_response(ctx, data, size,
        (uint16_t)(((uint16_t)data[0] << 8) | data[1]), "example.com", RATOS_DNS_A, "fuzzer", &result);
    ratos_dns_result_destroy(result); ratos_context_destroy(ctx); return 0;
}
