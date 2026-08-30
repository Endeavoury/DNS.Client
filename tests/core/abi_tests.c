#include <assert.h>
#include <string.h>
#include "ratatoskr/ratatoskr.h"

int main(void) {
    ratos_dns_query_options options;
    assert(ratos_abi_version() == RATOS_ABI_VERSION);
    assert(ratos_version_major() == RATOS_VERSION_MAJOR);
    assert(strcmp(ratos_error_string(RATOS_ERROR_TIMEOUT), "operation timed out") == 0);
    ratos_dns_query_options_init(&options);
    assert(options.struct_size == sizeof(options));
    assert(options.type == RATOS_DNS_A && options.port == 53u && options.timeout_ms == 5000u);
    ratos_context_destroy(NULL); ratos_dns_result_destroy(NULL);
    return 0;
}
