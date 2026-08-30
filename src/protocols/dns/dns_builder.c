#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include "protocols/dns/dns_internal.h"

static void write_u16(uint8_t *data, size_t offset, uint16_t value) {
    data[offset] = (uint8_t)(value >> 8);
    data[offset + 1u] = (uint8_t)value;
}

static int parse_address(int family, const char *text, uint8_t *bytes) {
#ifdef _WIN32
    WSADATA data;
    int parsed;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 0;
    parsed = inet_pton(family, text, bytes);
    (void)WSACleanup();
    return parsed;
#else
    return inet_pton(family, text, bytes);
#endif
}

static ratos_error reverse_name(ratos_context *ctx, const char *name, char **output) {
    uint8_t bytes[16];
    char buffer[128];
    size_t used = 0u;
    int i;
    (void)ctx;
    if (parse_address(AF_INET, name, bytes) == 1) {
        (void)snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u.in-addr.arpa",
            (unsigned)bytes[3], (unsigned)bytes[2], (unsigned)bytes[1], (unsigned)bytes[0]);
        *output = ratos_strdup(buffer);
        return *output != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
    }
    if (parse_address(AF_INET6, name, bytes) == 1) {
        for (i = 15; i >= 0; --i) {
            int wrote = snprintf(buffer + used, sizeof(buffer) - used, "%x.%x.",
                bytes[i] & 0x0f, bytes[i] >> 4);
            if (wrote < 0 || (size_t)wrote >= sizeof(buffer) - used) return RATOS_ERROR_GENERIC;
            used += (size_t)wrote;
        }
        (void)snprintf(buffer + used, sizeof(buffer) - used, "ip6.arpa");
        *output = ratos_strdup(buffer);
        return *output != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
    }
    *output = ratos_strdup(name);
    return *output != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
}

static ratos_error encode_name(ratos_context *ctx, const char *name, uint8_t *data,
    size_t capacity, size_t *position) {
    const unsigned char *cursor = (const unsigned char *)name;
    size_t wire_length = 1u, length_offset, label_length = 0u;
    if (strcmp(name, ".") == 0 || name[0] == '\0') {
        if (*position >= capacity) return RATOS_ERROR_PROTOCOL;
        data[(*position)++] = 0u;
        return RATOS_OK;
    }
    if (*position >= capacity) return RATOS_ERROR_PROTOCOL;
    length_offset = (*position)++;
    while (*cursor != 0u) {
        uint8_t octet;
        if (*cursor == (unsigned char)'.') {
            if (label_length == 0u) { ratos_set_error(ctx, "DNS name contains an empty label"); return RATOS_ERROR_INVALID_ARGUMENT; }
            data[length_offset] = (uint8_t)label_length;
            wire_length += label_length + 1u;
            ++cursor;
            if (*cursor == 0u) { label_length = 0u; break; }
            if (*position >= capacity) return RATOS_ERROR_PROTOCOL;
            length_offset = (*position)++; label_length = 0u;
            continue;
        }
        if (*cursor == (unsigned char)'\\') {
            ++cursor;
            if (*cursor == 0u) { ratos_set_error(ctx, "DNS name contains an incomplete escape"); return RATOS_ERROR_INVALID_ARGUMENT; }
            if (cursor[0] >= '0' && cursor[0] <= '9' && cursor[1] >= '0' && cursor[1] <= '9'
                && cursor[2] >= '0' && cursor[2] <= '9') {
                unsigned value = (unsigned)(cursor[0] - '0') * 100u + (unsigned)(cursor[1] - '0') * 10u + (unsigned)(cursor[2] - '0');
                if (value > 255u) { ratos_set_error(ctx, "DNS decimal escape exceeds 255"); return RATOS_ERROR_INVALID_ARGUMENT; }
                octet = (uint8_t)value; cursor += 3;
            } else octet = *cursor++;
        } else octet = *cursor++;
        if (label_length >= 63u || wire_length + label_length + 2u > 255u || *position >= capacity) {
            ratos_set_error(ctx, "DNS name exceeds RFC 1035 label or name limits"); return RATOS_ERROR_INVALID_ARGUMENT;
        }
        data[(*position)++] = octet; ++label_length;
    }
    if (label_length != 0u) { data[length_offset] = (uint8_t)label_length; wire_length += label_length + 1u; }
    if (wire_length > 255u || *position >= capacity) return RATOS_ERROR_INVALID_ARGUMENT;
    data[(*position)++] = 0u;
    return RATOS_OK;
}

ratos_error ratos_dns_build_query(ratos_context *ctx, const char *name,
    ratos_dns_type type, uint8_t recursion_desired, uint16_t id,
    ratos_dns_packet *packet, char **effective_name) {
    uint8_t *data;
    size_t position = RATOS_DNS_HEADER_SIZE;
    ratos_error error;
    char *query_name = NULL;
    if (ctx == NULL || name == NULL || name[0] == '\0' || packet == NULL || effective_name == NULL) return RATOS_ERROR_INVALID_ARGUMENT;
    if ((uint16_t)type == 0u) return RATOS_ERROR_INVALID_ARGUMENT;
    if (type == RATOS_DNS_PTR) {
        error = reverse_name(ctx, name, &query_name);
        if (error != RATOS_OK) return error;
    } else query_name = ratos_strdup(name);
    if (query_name == NULL) return RATOS_ERROR_OUT_OF_MEMORY;
    data = (uint8_t *)calloc(1u, 512u);
    if (data == NULL) { free(query_name); return RATOS_ERROR_OUT_OF_MEMORY; }
    write_u16(data, 0u, id);
    write_u16(data, 2u, recursion_desired != 0u ? 0x0100u : 0u);
    write_u16(data, 4u, 1u);
    error = encode_name(ctx, query_name, data, 512u, &position);
    if (error != RATOS_OK) { free(query_name); free(data); return error; }
    write_u16(data, position, (uint16_t)type); position += 2u;
    write_u16(data, position, 1u); position += 2u;
    packet->data = data;
    packet->length = position;
    *effective_name = query_name;
    return RATOS_OK;
}
