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

typedef struct dns_reader {
    ratos_context *ctx;
    const uint8_t *data;
    size_t length;
    size_t position;
} dns_reader;

static const char *format_address(int family, const uint8_t *bytes, char *output, size_t capacity) {
#ifdef _WIN32
    WSADATA data;
    const char *formatted;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return NULL;
    formatted = inet_ntop(family, bytes, output, capacity);
    (void)WSACleanup();
    return formatted != NULL ? output : NULL;
#else
    return inet_ntop(family, bytes, output, (socklen_t)capacity);
#endif
}

static uint16_t peek_u16(const uint8_t *data) {
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static ratos_error require(dns_reader *reader, size_t count) {
    if (count > reader->length - reader->position) {
        ratos_set_error(reader->ctx, "DNS packet is truncated at offset %zu", reader->position);
        return RATOS_ERROR_PROTOCOL;
    }
    return RATOS_OK;
}

static ratos_error read_u16(dns_reader *reader, uint16_t *value) {
    if (require(reader, 2u) != RATOS_OK) return RATOS_ERROR_PROTOCOL;
    *value = peek_u16(reader->data + reader->position);
    reader->position += 2u;
    return RATOS_OK;
}

static ratos_error read_u32(dns_reader *reader, uint32_t *value) {
    if (require(reader, 4u) != RATOS_OK) return RATOS_ERROR_PROTOCOL;
    *value = ((uint32_t)reader->data[reader->position] << 24)
        | ((uint32_t)reader->data[reader->position + 1u] << 16)
        | ((uint32_t)reader->data[reader->position + 2u] << 8)
        | reader->data[reader->position + 3u];
    reader->position += 4u;
    return RATOS_OK;
}

static ratos_error append_name_octet(char *output, size_t capacity, size_t *used, uint8_t value) {
    if (value >= 0x21u && value <= 0x7eu && value != (uint8_t)'.' && value != (uint8_t)'\\') {
        if (*used + 1u >= capacity) return RATOS_ERROR_PROTOCOL;
        output[(*used)++] = (char)value;
    } else if (value == (uint8_t)'.' || value == (uint8_t)'\\') {
        if (*used + 2u >= capacity) return RATOS_ERROR_PROTOCOL;
        output[(*used)++] = '\\'; output[(*used)++] = (char)value;
    } else {
        int wrote;
        if (*used + 4u >= capacity) return RATOS_ERROR_PROTOCOL;
        wrote = snprintf(output + *used, capacity - *used, "\\%03u", (unsigned)value);
        if (wrote != 4) return RATOS_ERROR_PROTOCOL;
        *used += 4u;
    }
    return RATOS_OK;
}

static ratos_error read_name(dns_reader *reader, char **name) {
    char output[1025];
    uint8_t visited[RATOS_DNS_MAX_PACKET / 8u + 1u] = {0};
    size_t cursor = reader->position, resume = 0u, used = 0u, wire_length = 1u, hops = 0u;
    int jumped = 0;
    while (1) {
        uint8_t label_length;
        size_t i;
        if (cursor >= reader->length) goto malformed;
        label_length = reader->data[cursor++];
        if ((label_length & 0xc0u) == 0xc0u) {
            size_t pointer;
            size_t location = cursor - 1u;
            if (cursor >= reader->length) goto malformed;
            pointer = ((size_t)(label_length & 0x3fu) << 8) | reader->data[cursor++];
            if (pointer >= reader->length || pointer >= location || hops++ >= 128u) goto malformed;
            if ((visited[pointer / 8u] & (uint8_t)(1u << (pointer % 8u))) != 0u) goto malformed;
            visited[pointer / 8u] |= (uint8_t)(1u << (pointer % 8u));
            if (!jumped) { resume = cursor; jumped = 1; }
            cursor = pointer;
            continue;
        }
        if ((label_length & 0xc0u) != 0u || label_length > 63u) goto malformed;
        if (label_length == 0u) {
            reader->position = jumped ? resume : cursor;
            if (used == 0u) output[used++] = '.';
            output[used] = '\0';
            *name = ratos_strdup(output);
            return *name != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
        }
        if ((size_t)label_length > reader->length - cursor || wire_length + (size_t)label_length + 1u > 255u) goto malformed;
        if (used != 0u) output[used++] = '.';
        for (i = 0u; i < label_length; ++i)
            if (append_name_octet(output, sizeof(output), &used, reader->data[cursor + i]) != RATOS_OK) goto malformed;
        cursor += label_length;
        wire_length += (size_t)label_length + 1u;
    }
malformed:
    ratos_set_error(reader->ctx, "Malformed or cyclic DNS name at offset %zu", reader->position);
    return RATOS_ERROR_PROTOCOL;
}

static int names_equal(const char *left, const char *right) {
    size_t a = strlen(left), b = strlen(right), i;
    while (a > 1u && left[a - 1u] == '.') --a;
    while (b > 1u && right[b - 1u] == '.') --b;
    if (a != b) return 0;
    for (i = 0u; i < a; ++i)
        if (tolower((unsigned char)left[i]) != tolower((unsigned char)right[i])) return 0;
    return 1;
}

static char *format_raw(const uint8_t *data, size_t length) {
    size_t i, capacity;
    char *value;
    int prefix;
    if (length > (SIZE_MAX - 32u) / 2u) return NULL;
    capacity = length * 2u + 32u;
    value = (char *)malloc(capacity);
    if (value == NULL) return NULL;
    prefix = snprintf(value, capacity, "\\# %zu ", length);
    if (prefix < 0) { free(value); return NULL; }
    for (i = 0u; i < length; ++i)
        (void)snprintf(value + (size_t)prefix + i * 2u, capacity - (size_t)prefix - i * 2u, "%02x", data[i]);
    return value;
}

static char *escape_text(const uint8_t *data, size_t length, int quoted) {
    size_t capacity, used = 0u, i;
    char *value;
    if (length > (SIZE_MAX - 8u) / 4u) return NULL;
    capacity = length * 4u + 8u;
    value = (char *)malloc(capacity);
    if (value == NULL) return NULL;
    if (quoted) value[used++] = '"';
    for (i = 0u; i < length; ++i) {
        uint8_t octet = data[i];
        if (octet >= 0x20u && octet <= 0x7eu && octet != (uint8_t)'"' && octet != (uint8_t)'\\') value[used++] = (char)octet;
        else if (octet == (uint8_t)'"' || octet == (uint8_t)'\\') { value[used++] = '\\'; value[used++] = (char)octet; }
        else { (void)snprintf(value + used, capacity - used, "\\%03u", (unsigned)octet); used += 4u; }
    }
    if (quoted) value[used++] = '"';
    value[used] = '\0';
    return value;
}

static ratos_error append_string_field(struct ratos_dns_record *record, const char *value) {
    char **items;
    items = (char **)realloc(record->strings, (record->string_count + 1u) * sizeof(*items));
    if (items == NULL) return RATOS_ERROR_OUT_OF_MEMORY;
    record->strings = items;
    record->strings[record->string_count] = ratos_strdup(value);
    if (record->strings[record->string_count] == NULL) return RATOS_ERROR_OUT_OF_MEMORY;
    ++record->string_count;
    return RATOS_OK;
}

static ratos_error read_character_string(dns_reader *reader, size_t end, char **value) {
    uint8_t length;
    if (reader->position >= end) return RATOS_ERROR_PROTOCOL;
    length = reader->data[reader->position++];
    if ((size_t)length > end - reader->position) return RATOS_ERROR_PROTOCOL;
    *value = escape_text(reader->data + reader->position, length, 0);
    reader->position += length;
    return *value != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
}

static ratos_error parse_txt(dns_reader *reader, struct ratos_dns_record *record, size_t end, char **text) {
    size_t capacity = (end - reader->position) * 4u + 16u, used = 0u;
    char *output = (char *)malloc(capacity);
    if (output == NULL) return RATOS_ERROR_OUT_OF_MEMORY;
    if (reader->position == end) { free(output); return RATOS_ERROR_PROTOCOL; }
    while (reader->position < end) {
        uint8_t length = reader->data[reader->position++];
        char *part;
        size_t part_length;
        if ((size_t)length > end - reader->position) { free(output); return RATOS_ERROR_PROTOCOL; }
        part = escape_text(reader->data + reader->position, length, 1);
        if (part == NULL) { free(output); return RATOS_ERROR_OUT_OF_MEMORY; }
        {
            char *plain = escape_text(reader->data + reader->position, length, 0);
            if (plain == NULL || append_string_field(record, plain) != RATOS_OK) { free(plain); free(part); free(output); return RATOS_ERROR_OUT_OF_MEMORY; }
            free(plain);
        }
        part_length = strlen(part);
        if (used != 0u) output[used++] = ' ';
        memcpy(output + used, part, part_length); used += part_length;
        free(part);
        reader->position += length;
    }
    output[used] = '\0'; *text = output;
    return RATOS_OK;
}

static ratos_error parse_record_text(dns_reader *reader, struct ratos_dns_record *record, uint16_t class_code, size_t end) {
    char *first = NULL, *second = NULL, address[INET6_ADDRSTRLEN];
    uint16_t a, b, c;
    uint32_t one, two, three, four, five;
    int needed;
    if (class_code != 1u && (record->type == RATOS_DNS_A || record->type == RATOS_DNS_AAAA || record->type == RATOS_DNS_WKS)) {
        reader->position = end; record->text = format_raw(record->raw_data, record->raw_length);
        return record->text != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
    }
    switch (record->type) {
    case RATOS_DNS_A:
        if (end - reader->position != 4u || format_address(AF_INET, reader->data + reader->position, address, sizeof(address)) == NULL) return RATOS_ERROR_PROTOCOL;
        reader->position = end; record->text = ratos_strdup(address); break;
    case RATOS_DNS_AAAA:
        if (end - reader->position != 16u || format_address(AF_INET6, reader->data + reader->position, address, sizeof(address)) == NULL) return RATOS_ERROR_PROTOCOL;
        reader->position = end; record->text = ratos_strdup(address); break;
    case RATOS_DNS_NS: case RATOS_DNS_MD: case RATOS_DNS_MF: case RATOS_DNS_CNAME:
    case RATOS_DNS_MB: case RATOS_DNS_MG: case RATOS_DNS_MR: case RATOS_DNS_PTR:
        if (read_name(reader, &first) != RATOS_OK || reader->position != end) { free(first); return RATOS_ERROR_PROTOCOL; }
        if (append_string_field(record, first) != RATOS_OK) { free(first); return RATOS_ERROR_OUT_OF_MEMORY; }
        record->text = first; break;
    case RATOS_DNS_MX:
        if (read_u16(reader, &a) != RATOS_OK || read_name(reader, &first) != RATOS_OK || reader->position != end) { free(first); return RATOS_ERROR_PROTOCOL; }
        record->values16[0] = a; record->values16_count = 1u;
        if (append_string_field(record, first) != RATOS_OK) { free(first); return RATOS_ERROR_OUT_OF_MEMORY; }
        needed = snprintf(NULL, 0, "%u %s", (unsigned)a, first);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "%u %s", (unsigned)a, first);
        free(first); break;
    case RATOS_DNS_HINFO:
        if (read_character_string(reader, end, &first) != RATOS_OK
            || read_character_string(reader, end, &second) != RATOS_OK || reader->position != end) {
            free(first); free(second); return RATOS_ERROR_PROTOCOL;
        }
        if (append_string_field(record, first) != RATOS_OK || append_string_field(record, second) != RATOS_OK) {
            free(first); free(second); return RATOS_ERROR_OUT_OF_MEMORY;
        }
        needed = snprintf(NULL, 0, "\"%s\" \"%s\"", first, second);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "\"%s\" \"%s\"", first, second);
        free(first); free(second); break;
    case RATOS_DNS_MINFO:
        if (read_name(reader, &first) != RATOS_OK || read_name(reader, &second) != RATOS_OK || reader->position != end) {
            free(first); free(second); return RATOS_ERROR_PROTOCOL;
        }
        if (append_string_field(record, first) != RATOS_OK || append_string_field(record, second) != RATOS_OK) {
            free(first); free(second); return RATOS_ERROR_OUT_OF_MEMORY;
        }
        needed = snprintf(NULL, 0, "%s %s", first, second);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "%s %s", first, second);
        free(first); free(second); break;
    case RATOS_DNS_WKS:
        if (end - reader->position < 5u || format_address(AF_INET, reader->data + reader->position, address, sizeof(address)) == NULL)
            return RATOS_ERROR_PROTOCOL;
        reader->position += 4u; a = reader->data[reader->position++];
        record->values16[0] = a; record->values16_count = 1u;
        if (append_string_field(record, address) != RATOS_OK) return RATOS_ERROR_OUT_OF_MEMORY;
        record->text = format_raw(record->raw_data, record->raw_length); reader->position = end; break;
    case RATOS_DNS_TXT:
        return parse_txt(reader, record, end, &record->text);
    case RATOS_DNS_SRV:
        if (read_u16(reader, &a) != RATOS_OK || read_u16(reader, &b) != RATOS_OK || read_u16(reader, &c) != RATOS_OK || read_name(reader, &first) != RATOS_OK || reader->position != end) { free(first); return RATOS_ERROR_PROTOCOL; }
        record->values16[0] = a; record->values16[1] = b; record->values16[2] = c; record->values16_count = 3u;
        if (append_string_field(record, first) != RATOS_OK) { free(first); return RATOS_ERROR_OUT_OF_MEMORY; }
        needed = snprintf(NULL, 0, "%u %u %u %s", (unsigned)a, (unsigned)b, (unsigned)c, first);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "%u %u %u %s", (unsigned)a, (unsigned)b, (unsigned)c, first);
        free(first); break;
    case RATOS_DNS_SOA:
        if (read_name(reader, &first) != RATOS_OK || read_name(reader, &second) != RATOS_OK
            || read_u32(reader, &one) != RATOS_OK || read_u32(reader, &two) != RATOS_OK
            || read_u32(reader, &three) != RATOS_OK || read_u32(reader, &four) != RATOS_OK
            || read_u32(reader, &five) != RATOS_OK || reader->position != end) { free(first); free(second); return RATOS_ERROR_PROTOCOL; }
        if (append_string_field(record, first) != RATOS_OK || append_string_field(record, second) != RATOS_OK) { free(first); free(second); return RATOS_ERROR_OUT_OF_MEMORY; }
        record->values32[0]=one; record->values32[1]=two; record->values32[2]=three; record->values32[3]=four; record->values32[4]=five; record->values32_count=5u;
        needed = snprintf(NULL, 0, "%s %s %u %u %u %u %u", first, second, one, two, three, four, five);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "%s %s %u %u %u %u %u", first, second, one, two, three, four, five);
        free(first); free(second); break;
    case RATOS_DNS_NAPTR:
        {
            char *third = NULL, *fourth = NULL;
            if (read_u16(reader, &a) != RATOS_OK || read_u16(reader, &b) != RATOS_OK
                || read_character_string(reader, end, &first) != RATOS_OK
                || read_character_string(reader, end, &second) != RATOS_OK
                || read_character_string(reader, end, &third) != RATOS_OK
                || read_name(reader, &fourth) != RATOS_OK || reader->position != end) {
                free(first); free(second); free(third); free(fourth); return RATOS_ERROR_PROTOCOL;
            }
            record->values16[0] = a; record->values16[1] = b; record->values16_count = 2u;
            if (append_string_field(record, first) != RATOS_OK || append_string_field(record, second) != RATOS_OK
                || append_string_field(record, third) != RATOS_OK || append_string_field(record, fourth) != RATOS_OK) {
                free(first); free(second); free(third); free(fourth); return RATOS_ERROR_OUT_OF_MEMORY;
            }
            needed = snprintf(NULL, 0, "%u %u \"%s\" \"%s\" \"%s\" %s",
                (unsigned)a, (unsigned)b, first, second, third, fourth);
            record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
            if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u,
                "%u %u \"%s\" \"%s\" \"%s\" %s", (unsigned)a, (unsigned)b, first, second, third, fourth);
            free(first); free(second); free(third); free(fourth);
        }
        break;
    case RATOS_DNS_CAA:
        if (end - reader->position < 2u) return RATOS_ERROR_PROTOCOL;
        a = reader->data[reader->position++]; b = reader->data[reader->position++];
        if ((size_t)b > end - reader->position) return RATOS_ERROR_PROTOCOL;
        first = escape_text(reader->data + reader->position, b, 0); reader->position += b;
        second = escape_text(reader->data + reader->position, end - reader->position, 1); reader->position = end;
        if (first == NULL || second == NULL) { free(first); free(second); return RATOS_ERROR_OUT_OF_MEMORY; }
        record->values16[0] = a; record->values16_count = 1u;
        if (append_string_field(record, first) != RATOS_OK || append_string_field(record, second) != RATOS_OK) { free(first); free(second); return RATOS_ERROR_OUT_OF_MEMORY; }
        needed = snprintf(NULL, 0, "%u %s %s", (unsigned)a, first, second);
        record->text = needed >= 0 ? (char *)malloc((size_t)needed + 1u) : NULL;
        if (record->text != NULL) (void)snprintf(record->text, (size_t)needed + 1u, "%u %s %s", (unsigned)a, first, second);
        free(first); free(second); break;
    default:
        reader->position = end;
        record->text = format_raw(record->raw_data, record->raw_length);
        break;
    }
    return record->text != NULL ? RATOS_OK : RATOS_ERROR_OUT_OF_MEMORY;
}

static ratos_error read_records(dns_reader *reader, uint16_t count, ratos_dns_section section,
    ratos_dns_result *result, size_t *index) {
    uint16_t i;
    for (i = 0u; i < count; ++i) {
        struct ratos_dns_record *record = &result->records[*index];
        uint16_t class_code, rdlength;
        size_t end;
        ratos_error error;
        record->section = section;
        if (read_name(reader, &record->name) != RATOS_OK || read_u16(reader, &record->type) != RATOS_OK
            || read_u16(reader, &class_code) != RATOS_OK || read_u32(reader, &record->ttl) != RATOS_OK
            || read_u16(reader, &rdlength) != RATOS_OK) return RATOS_ERROR_PROTOCOL;
        (void)class_code;
        if ((size_t)rdlength > reader->length - reader->position) { ratos_set_error(reader->ctx, "DNS RDLENGTH exceeds packet bounds"); return RATOS_ERROR_PROTOCOL; }
        end = reader->position + rdlength;
        record->raw_length = rdlength;
        if (rdlength != 0u) {
            record->raw_data = (uint8_t *)malloc(rdlength);
            if (record->raw_data == NULL) return RATOS_ERROR_OUT_OF_MEMORY;
            memcpy(record->raw_data, reader->data + reader->position, rdlength);
        }
        error = parse_record_text(reader, record, class_code, end);
        if (error != RATOS_OK || reader->position != end) {
            if (error == RATOS_ERROR_PROTOCOL) ratos_set_error(reader->ctx, "Malformed %s RDATA", ratos_dns_type_string(record->type));
            return error;
        }
        ++(*index);
    }
    return RATOS_OK;
}

ratos_error ratos_dns_parse_response(ratos_context *ctx, const uint8_t *data,
    size_t length, uint16_t expected_id, const char *expected_name,
    ratos_dns_type expected_type, const char *server, ratos_dns_result **out_result) {
    dns_reader reader;
    ratos_dns_result *result = NULL;
    uint16_t id, flags, qd, an, ns, ar, qtype, qclass;
    size_t total, index = 0u;
    char *question = NULL;
    ratos_error error = RATOS_ERROR_PROTOCOL;
    if (out_result != NULL) *out_result = NULL;
    if (ctx == NULL || data == NULL || expected_name == NULL || server == NULL || out_result == NULL) return RATOS_ERROR_INVALID_ARGUMENT;
    if (length < RATOS_DNS_HEADER_SIZE || length > RATOS_DNS_MAX_PACKET) { ratos_set_error(ctx, "Invalid DNS packet length: %zu", length); return RATOS_ERROR_PROTOCOL; }
    reader.ctx = ctx; reader.data = data; reader.length = length; reader.position = 0u;
    if (read_u16(&reader, &id) != RATOS_OK || read_u16(&reader, &flags) != RATOS_OK
        || read_u16(&reader, &qd) != RATOS_OK || read_u16(&reader, &an) != RATOS_OK
        || read_u16(&reader, &ns) != RATOS_OK || read_u16(&reader, &ar) != RATOS_OK) return RATOS_ERROR_PROTOCOL;
    if (id != expected_id) { ratos_set_error(ctx, "DNS response transaction ID mismatch"); return RATOS_ERROR_PROTOCOL; }
    if ((flags & 0x8000u) == 0u || (flags & 0x0040u) != 0u || (flags & 0x7800u) != 0u) { ratos_set_error(ctx, "Invalid DNS response flags or opcode"); return RATOS_ERROR_PROTOCOL; }
    if (qd != 1u) { ratos_set_error(ctx, "DNS response must echo exactly one question"); return RATOS_ERROR_PROTOCOL; }
    total = (size_t)an + ns + ar;
    if (total > RATOS_DNS_MAX_RECORDS) { ratos_set_error(ctx, "DNS response exceeds record limit"); return RATOS_ERROR_PROTOCOL; }
    if (read_name(&reader, &question) != RATOS_OK || read_u16(&reader, &qtype) != RATOS_OK || read_u16(&reader, &qclass) != RATOS_OK) goto cleanup;
    if (!names_equal(question, expected_name) || qtype != (uint16_t)expected_type || qclass != 1u) { ratos_set_error(ctx, "DNS response question does not match query"); goto cleanup; }
    result = (ratos_dns_result *)calloc(1u, sizeof(*result));
    if (result == NULL) { error = RATOS_ERROR_OUT_OF_MEMORY; goto cleanup; }
    result->transaction_id = id; result->rcode = (uint8_t)(flags & 0x0fu); result->authoritative = (flags & 0x0400u) != 0u;
    result->truncated = (flags & 0x0200u) != 0u; result->query_type = expected_type;
    result->recursion_desired = (flags & 0x0100u) != 0u; result->recursion_available = (flags & 0x0080u) != 0u;
    result->authentic_data = (flags & 0x0020u) != 0u; result->checking_disabled = (flags & 0x0010u) != 0u;
    result->server = ratos_strdup(server); result->query_name = ratos_strdup(expected_name); result->count = total;
    if (result->server == NULL || result->query_name == NULL) { error = RATOS_ERROR_OUT_OF_MEMORY; goto cleanup; }
    if (total != 0u) {
        result->records = (struct ratos_dns_record *)calloc(total, sizeof(*result->records));
        if (result->records == NULL) { error = RATOS_ERROR_OUT_OF_MEMORY; goto cleanup; }
    }
    error = read_records(&reader, an, RATOS_DNS_SECTION_ANSWER, result, &index);
    if (error == RATOS_OK) error = read_records(&reader, ns, RATOS_DNS_SECTION_AUTHORITY, result, &index);
    if (error == RATOS_OK) error = read_records(&reader, ar, RATOS_DNS_SECTION_ADDITIONAL, result, &index);
    if (error == RATOS_OK && reader.position != length) { ratos_set_error(ctx, "DNS response has trailing bytes"); error = RATOS_ERROR_PROTOCOL; }
    if (error == RATOS_OK) { free(question); *out_result = result; return RATOS_OK; }
cleanup:
    free(question);
    if (result != NULL) { result->count = index < result->count ? index + 1u : result->count; ratos_dns_result_destroy(result); }
    return error;
}
