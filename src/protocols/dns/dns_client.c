#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#endif
#include "protocols/dns/dns_internal.h"

void ratos_dns_query_options_init(ratos_dns_query_options *options) {
    if (options == NULL) return;
    memset(options, 0, sizeof(*options));
    options->struct_size = (uint32_t)sizeof(*options);
    options->port = 53u; options->type = RATOS_DNS_A; options->timeout_ms = 5000u; options->recursion_desired = 1u;
}

char *ratos_dns_default_server(void) {
#ifdef _WIN32
    FIXED_INFO *info = NULL;
    ULONG length = 0u;
    if (GetNetworkParams(NULL, &length) == ERROR_BUFFER_OVERFLOW) info = (FIXED_INFO *)malloc(length);
    if (info != NULL && GetNetworkParams(info, &length) == NO_ERROR) {
        char *server = ratos_strdup(info->DnsServerList.IpAddress.String);
        free(info); return server;
    }
    free(info);
#else
    FILE *file = fopen("/etc/resolv.conf", "r");
    char line[512];
    if (file != NULL) {
        while (fgets(line, sizeof(line), file) != NULL) {
            char server[256];
            if (sscanf(line, " nameserver %255s", server) == 1 || sscanf(line, "nameserver %255s", server) == 1) {
                fclose(file); return ratos_strdup(server);
            }
        }
        fclose(file);
    }
#endif
    return NULL;
}

static uint16_t query_id(void) {
    uint16_t value = 0u;
#ifdef _WIN32
    unsigned int random_value = 0u;
    if (rand_s(&random_value) == 0) value = (uint16_t)random_value;
#else
    FILE *random_file = fopen("/dev/urandom", "rb");
    if (random_file != NULL) {
        if (fread(&value, sizeof(value), 1u, random_file) != 1u) value = 0u;
        fclose(random_file);
    }
#endif
    if (value == 0u) value = (uint16_t)((unsigned)time(NULL) ^ (unsigned)(uintptr_t)&value);
    return value;
}

static int response_requests_tcp(const uint8_t *data, size_t length, uint16_t expected_id) {
    uint16_t id, flags;
    if (length < 4u) return 0;
    id = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    flags = (uint16_t)(((uint16_t)data[2] << 8) | data[3]);
    return id == expected_id && (flags & 0x8000u) != 0u && (flags & 0x0200u) != 0u
        && (flags & 0x0040u) == 0u && (flags & 0x7800u) == 0u;
}

ratos_error ratos_dns_query(ratos_context *ctx, const char *name,
    const ratos_dns_query_options *provided, ratos_dns_result **out_result) {
    ratos_dns_query_options options;
    ratos_dns_packet query = {0};
    uint8_t *wire_response = NULL;
    size_t response_length = 0u;
    char *default_server = NULL, *effective_name = NULL;
    const char *server;
    uint16_t id;
    ratos_dns_result *udp_result = NULL;
    ratos_error error;
    if (out_result != NULL) *out_result = NULL;
    if (ctx == NULL || name == NULL || name[0] == '\0' || out_result == NULL) return RATOS_ERROR_INVALID_ARGUMENT;
    ctx->error_message[0] = '\0';
    ratos_dns_query_options_init(&options);
    if (provided != NULL) {
        if (provided->struct_size < offsetof(ratos_dns_query_options, reserved) + sizeof(provided->reserved)) {
            ratos_set_error(ctx, "DNS options struct is too small for ABI version 1"); return RATOS_ERROR_INVALID_ARGUMENT;
        }
        options = *provided;
        if (options.port == 0u) options.port = 53u;
        if (options.timeout_ms == 0u) options.timeout_ms = 5000u;
        if ((uint16_t)options.type == 0u) options.type = RATOS_DNS_A;
    }
    server = options.server;
    if (server == NULL || server[0] == '\0') {
        default_server = ratos_dns_default_server(); server = default_server;
        if (server == NULL) { ratos_set_error(ctx, "No system DNS resolver was found; specify a server"); return RATOS_ERROR_NETWORK; }
    }
    id = query_id();
    error = ratos_dns_build_query(ctx, name, options.type, options.recursion_desired, id, &query, &effective_name);
    if (error != RATOS_OK) goto cleanup;
    error = ratos_dns_udp_exchange(ctx, server, options.port, options.timeout_ms, query.data, query.length, &wire_response, &response_length);
    if (error != RATOS_OK) goto cleanup;
    if (response_requests_tcp(wire_response, response_length, id)) {
        free(wire_response); wire_response = NULL;
        error = ratos_dns_tcp_exchange(ctx, server, options.port, options.timeout_ms, query.data, query.length, &wire_response, &response_length);
        if (error != RATOS_OK) goto cleanup;
        error = ratos_dns_parse_response(ctx, wire_response, response_length, id, effective_name, options.type, server, &udp_result);
        if (error != RATOS_OK) goto cleanup;
    } else {
        error = ratos_dns_parse_response(ctx, wire_response, response_length, id, effective_name, options.type, server, &udp_result);
        if (error != RATOS_OK) goto cleanup;
    }
    *out_result = udp_result; udp_result = NULL;
cleanup:
    free(query.data); free(wire_response); free(effective_name); free(default_server); ratos_dns_result_destroy(udp_result);
    return error;
}
