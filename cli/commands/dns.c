#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ratatoskr/ratatoskr.h"

enum { EXIT_GENERAL = 1, EXIT_ARGUMENTS = 2, EXIT_TIMEOUT = 3,
    EXIT_NETWORK = 4, EXIT_DNS = 5, EXIT_UNSUPPORTED = 6, EXIT_PERMISSION = 7 };

static void dns_help(FILE *stream) {
    fprintf(stream,
        "Query DNS through the Ratatoskr native core.\n\n"
        "Usage:\n  ratos dns [TYPE] NAME [OPTIONS]\n\n"
        "Types:\n  a, aaaa, cname, mx, txt, ns, ptr, soa, srv, naptr, caa\n\n"
        "Options:\n"
        "  -s, --server ADDRESS   DNS resolver (IPv4 or IPv6)\n"
        "      --port PORT        Resolver port (default: 53)\n"
        "      --timeout MS       Timeout per transport (default: 5000)\n"
        "      --json             Stable JSON output\n"
        "      --verbose          Diagnostics on stderr\n"
        "  -h, --help             Show this help\n\n"
        "With no TYPE, A is used; an IP address is detected as PTR.\n");
}

static int parse_type(const char *value, ratos_dns_type *type) {
    struct entry { const char *name; ratos_dns_type type; } entries[] = {
        {"a", RATOS_DNS_A}, {"aaaa", RATOS_DNS_AAAA}, {"cname", RATOS_DNS_CNAME},
        {"mx", RATOS_DNS_MX}, {"txt", RATOS_DNS_TXT}, {"ns", RATOS_DNS_NS},
        {"ptr", RATOS_DNS_PTR}, {"soa", RATOS_DNS_SOA}, {"srv", RATOS_DNS_SRV},
        {"naptr", RATOS_DNS_NAPTR}, {"caa", RATOS_DNS_CAA}
    };
    size_t i;
    for (i = 0u; i < sizeof(entries) / sizeof(entries[0]); ++i)
        if (strcmp(value, entries[i].name) == 0) { *type = entries[i].type; return 1; }
    return 0;
}

static int appears_ip(const char *value) {
    size_t i;
    int dots = 0;
    if (strchr(value, ':') != NULL) return 1;
    for (i = 0u; value[i] != '\0'; ++i) {
        if (value[i] == '.') ++dots;
        else if (value[i] < '0' || value[i] > '9') return 0;
    }
    return dots == 3;
}

static int parse_u32(const char *value, uint32_t minimum, uint32_t maximum, uint32_t *output) {
    char *end = NULL;
    unsigned long parsed;
    errno = 0; parsed = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < minimum || parsed > maximum) return 0;
    *output = (uint32_t)parsed; return 1;
}

static void json_string(const char *value) {
    const unsigned char *cursor = (const unsigned char *)(value != NULL ? value : "");
    putchar('"');
    while (*cursor != 0u) {
        switch (*cursor) {
        case '"': fputs("\\\"", stdout); break; case '\\': fputs("\\\\", stdout); break;
        case '\b': fputs("\\b", stdout); break; case '\f': fputs("\\f", stdout); break;
        case '\n': fputs("\\n", stdout); break; case '\r': fputs("\\r", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        default:
            if (*cursor < 0x20u) printf("\\u%04x", (unsigned)*cursor); else putchar((int)*cursor);
        }
        ++cursor;
    }
    putchar('"');
}

static const char *section_name(ratos_dns_section section) {
    switch (section) { case RATOS_DNS_SECTION_ANSWER: return "answer";
    case RATOS_DNS_SECTION_AUTHORITY: return "authority";
    case RATOS_DNS_SECTION_ADDITIONAL: return "additional"; default: return "unknown"; }
}

static void print_json(const ratos_dns_result *result, const char *input_name) {
    size_t i;
    fputs("{\"protocol\":\"dns\",\"query\":{\"name\":", stdout);
    json_string(input_name);
    if (strcmp(input_name, ratos_dns_result_query_name(result)) != 0) {
        fputs(",\"wireName\":", stdout); json_string(ratos_dns_result_query_name(result));
    }
    fputs(",\"type\":", stdout); json_string(ratos_dns_type_string((uint16_t)ratos_dns_result_query_type(result)));
    fputs(",\"server\":", stdout); json_string(ratos_dns_result_server(result));
    fputs("},\"response\":{\"rcode\":", stdout); json_string(ratos_dns_rcode_string(ratos_dns_result_rcode(result)));
    printf(",\"authoritative\":%s,\"truncated\":%s,\"records\":[",
        ratos_dns_result_authoritative(result) ? "true" : "false",
        ratos_dns_result_truncated(result) ? "true" : "false");
    for (i = 0u; i < ratos_dns_result_count(result); ++i) {
        const ratos_dns_record *record = ratos_dns_result_record(result, i);
        uint16_t type = ratos_dns_record_type_code(record);
        uint16_t u16a = 0u, u16b = 0u, u16c = 0u;
        uint32_t u32a = 0u, u32b = 0u, u32c = 0u, u32d = 0u, u32e = 0u;
        if (i != 0u) putchar(',');
        fputs("{\"type\":", stdout); json_string(ratos_dns_type_string(type));
        printf(",\"typeCode\":%u", (unsigned)type);
        fputs(",\"name\":", stdout); json_string(ratos_dns_record_name(record));
        printf(",\"ttl\":%u,\"section\":", ratos_dns_record_ttl(record)); json_string(section_name(ratos_dns_record_section(record)));
        if (type == RATOS_DNS_A || type == RATOS_DNS_AAAA) {
            fputs(",\"address\":", stdout); json_string(ratos_dns_record_text(record));
        } else if (type == RATOS_DNS_NS || type == RATOS_DNS_CNAME || type == RATOS_DNS_PTR) {
            fputs(",\"target\":", stdout); json_string(ratos_dns_record_string(record, 0u));
        } else if (type == RATOS_DNS_MX && ratos_dns_record_uint16(record, 0u, &u16a)) {
            printf(",\"preference\":%u,\"exchange\":", (unsigned)u16a); json_string(ratos_dns_record_string(record, 0u));
        } else if (type == RATOS_DNS_TXT) {
            size_t string_index, string_count = ratos_dns_record_string_count(record);
            fputs(",\"strings\":[", stdout);
            for (string_index = 0u; string_index < string_count; ++string_index) {
                if (string_index != 0u) putchar(',');
                json_string(ratos_dns_record_string(record, string_index));
            }
            putchar(']');
        } else if (type == RATOS_DNS_SRV && ratos_dns_record_uint16(record, 0u, &u16a)
            && ratos_dns_record_uint16(record, 1u, &u16b) && ratos_dns_record_uint16(record, 2u, &u16c)) {
            printf(",\"priority\":%u,\"weight\":%u,\"port\":%u,\"target\":",
                (unsigned)u16a, (unsigned)u16b, (unsigned)u16c); json_string(ratos_dns_record_string(record, 0u));
        } else if (type == RATOS_DNS_SOA && ratos_dns_record_uint32(record,0u,&u32a)
            && ratos_dns_record_uint32(record,1u,&u32b) && ratos_dns_record_uint32(record,2u,&u32c)
            && ratos_dns_record_uint32(record,3u,&u32d) && ratos_dns_record_uint32(record,4u,&u32e)) {
            fputs(",\"primaryNameServer\":",stdout); json_string(ratos_dns_record_string(record,0u));
            fputs(",\"responsibleMailbox\":",stdout); json_string(ratos_dns_record_string(record,1u));
            printf(",\"serial\":%u,\"refresh\":%u,\"retry\":%u,\"expire\":%u,\"minimum\":%u",
                (unsigned)u32a,(unsigned)u32b,(unsigned)u32c,(unsigned)u32d,(unsigned)u32e);
        } else if (type == RATOS_DNS_CAA && ratos_dns_record_uint16(record,0u,&u16a)) {
            printf(",\"flags\":%u,\"tag\":",(unsigned)u16a); json_string(ratos_dns_record_string(record,0u));
            fputs(",\"value\":",stdout); json_string(ratos_dns_record_string(record,1u));
        } else if (type == RATOS_DNS_NAPTR && ratos_dns_record_uint16(record,0u,&u16a)
            && ratos_dns_record_uint16(record,1u,&u16b)) {
            printf(",\"order\":%u,\"preference\":%u,\"flags\":",(unsigned)u16a,(unsigned)u16b);
            json_string(ratos_dns_record_string(record,0u)); fputs(",\"services\":",stdout); json_string(ratos_dns_record_string(record,1u));
            fputs(",\"regexp\":",stdout); json_string(ratos_dns_record_string(record,2u));
            fputs(",\"replacement\":",stdout); json_string(ratos_dns_record_string(record,3u));
        } else { fputs(",\"data\":", stdout); json_string(ratos_dns_record_text(record)); }
        putchar('}');
    }
    fputs("]}}\n", stdout);
}

static int map_error(ratos_error error) {
    switch (error) { case RATOS_ERROR_INVALID_ARGUMENT: return EXIT_ARGUMENTS;
    case RATOS_ERROR_TIMEOUT: return EXIT_TIMEOUT; case RATOS_ERROR_NETWORK: return EXIT_NETWORK;
    case RATOS_ERROR_DNS: case RATOS_ERROR_PROTOCOL: case RATOS_ERROR_NOT_FOUND: return EXIT_DNS;
    case RATOS_ERROR_UNSUPPORTED: return EXIT_UNSUPPORTED; case RATOS_ERROR_PERMISSION_DENIED: return EXIT_PERMISSION;
    default: return EXIT_GENERAL; }
}

int ratos_cli_dns(int argc, char **argv) {
    ratos_dns_query_options options;
    ratos_dns_result *result = NULL;
    ratos_context *ctx;
    const char *name = NULL;
    int json = 0, verbose = 0, explicit_type = 0, i;
    ratos_error error;
    ratos_dns_query_options_init(&options);
    for (i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) { dns_help(stdout); return 0; }
        if (strcmp(argv[i], "--json") == 0) { json = 1; continue; }
        if (strcmp(argv[i], "--verbose") == 0) { verbose = 1; continue; }
        if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "-s") == 0) {
            if (++i >= argc) { fputs("ratos dns: --server requires an address\n", stderr); return EXIT_ARGUMENTS; }
            options.server = argv[i]; continue;
        }
        if (strcmp(argv[i], "--port") == 0) {
            uint32_t port;
            if (++i >= argc || !parse_u32(argv[i], 1u, 65535u, &port)) { fputs("ratos dns: invalid port\n", stderr); return EXIT_ARGUMENTS; }
            options.port = (uint16_t)port; continue;
        }
        if (strcmp(argv[i], "--timeout") == 0) {
            if (++i >= argc || !parse_u32(argv[i], 1u, UINT32_MAX, &options.timeout_ms)) { fputs("ratos dns: invalid timeout\n", stderr); return EXIT_ARGUMENTS; }
            continue;
        }
        if (argv[i][0] == '-') { fprintf(stderr, "ratos dns: unknown option '%s'\n", argv[i]); return EXIT_ARGUMENTS; }
        if (name == NULL && parse_type(argv[i], &options.type)) { explicit_type = 1; continue; }
        if (name == NULL) { name = argv[i]; continue; }
        fprintf(stderr, "ratos dns: unexpected argument '%s'\n", argv[i]); return EXIT_ARGUMENTS;
    }
    if (name == NULL) { dns_help(stderr); return EXIT_ARGUMENTS; }
    if (!explicit_type && appears_ip(name)) options.type = RATOS_DNS_PTR;
    ctx = ratos_context_create();
    if (ctx == NULL) { fputs("ratos dns: out of memory\n", stderr); return EXIT_GENERAL; }
    if (verbose) fprintf(stderr, "ratos: DNS %s query for %s%s%s\n", ratos_dns_type_string((uint16_t)options.type), name,
        options.server != NULL ? " via " : "", options.server != NULL ? options.server : "");
    error = ratos_dns_query(ctx, name, &options, &result);
    if (error != RATOS_OK) {
        fprintf(stderr, "ratos dns: %s%s%s\n", ratos_error_string(error), ratos_context_error(ctx)[0] ? ": " : "", ratos_context_error(ctx));
        ratos_context_destroy(ctx); return map_error(error);
    }
    if (json) print_json(result, name);
    else {
        size_t index;
        for (index = 0u; index < ratos_dns_result_count(result); ++index) {
            const ratos_dns_record *record = ratos_dns_result_record(result, index);
            if (ratos_dns_record_section(record) == RATOS_DNS_SECTION_ANSWER) puts(ratos_dns_record_text(record));
        }
    }
    if (verbose) fprintf(stderr, "ratos: response %s with %zu record(s)\n",
        ratos_dns_rcode_string(ratos_dns_result_rcode(result)), ratos_dns_result_count(result));
    i = ratos_dns_result_rcode(result) == 0u ? 0 : EXIT_DNS;
    ratos_dns_result_destroy(result); ratos_context_destroy(ctx); return i;
}
