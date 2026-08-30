#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocols/dns/dns_internal.h"
#include "ratatoskr/version.h"

typedef struct packet_writer { uint8_t data[2048]; size_t length; } packet_writer;
static void u16(packet_writer *w, uint16_t value) { w->data[w->length++] = (uint8_t)(value >> 8); w->data[w->length++] = (uint8_t)value; }
static void u32(packet_writer *w, uint32_t value) { u16(w, (uint16_t)(value >> 16)); u16(w, (uint16_t)value); }
static void bytes(packet_writer *w, const uint8_t *value, size_t length) { memcpy(w->data + w->length, value, length); w->length += length; }
static void name(packet_writer *w, const char *value) {
    const char *cursor = value;
    while (*cursor != '\0') {
        const char *dot = strchr(cursor, '.'); size_t length = dot != NULL ? (size_t)(dot - cursor) : strlen(cursor);
        w->data[w->length++] = (uint8_t)length; bytes(w, (const uint8_t *)cursor, length);
        if (dot == NULL) break;
        cursor = dot + 1;
    }
    w->data[w->length++] = 0u;
}
static packet_writer response(uint16_t qtype, uint16_t answer_type, const uint8_t *rdata, size_t rdlength) {
    packet_writer w = {{0}, 0u};
    u16(&w, 0x1234u); u16(&w, 0x8180u); u16(&w, 1u); u16(&w, 1u); u16(&w, 0u); u16(&w, 0u);
    name(&w, "example.com"); u16(&w, qtype); u16(&w, 1u);
    u16(&w, 0xc00cu); u16(&w, answer_type); u16(&w, 1u); u32(&w, 300u); u16(&w, (uint16_t)rdlength); bytes(&w, rdata, rdlength);
    return w;
}
static void expect_text(uint16_t qtype, uint16_t answer_type, const uint8_t *rdata, size_t length, const char *expected) {
    packet_writer wire = response(qtype, answer_type, rdata, length);
    ratos_context *ctx = ratos_context_create(); ratos_dns_result *result = NULL;
    assert(ratos_dns_parse_response(ctx, wire.data, wire.length, 0x1234u, "example.com",
        (ratos_dns_type)qtype, "fixture", &result) == RATOS_OK);
    assert(result != NULL && ratos_dns_result_count(result) == 1u);
    assert(ratos_dns_result_transaction_id(result) == 0x1234u);
    assert(ratos_dns_result_recursion_desired(result) != 0u);
    assert(ratos_dns_result_recursion_available(result) != 0u);
    assert(strcmp(ratos_dns_record_text(ratos_dns_result_record(result, 0u)), expected) == 0);
    ratos_dns_result_destroy(result); ratos_context_destroy(ctx);
}

static void test_builder(void) {
    ratos_context *ctx = ratos_context_create(); ratos_dns_packet packet = {0}; char *effective = NULL;
    assert(ratos_dns_build_query(ctx, "example.com", RATOS_DNS_A, 1u, 0xa05cu, &packet, &effective) == RATOS_OK);
    assert(packet.length == 29u && packet.data[0] == 0xa0u && packet.data[1] == 0x5cu);
    assert(strcmp(effective, "example.com") == 0); free(packet.data); free(effective);
    packet.data = NULL; effective = NULL;
    assert(ratos_dns_build_query(ctx, "2001:db8::1", RATOS_DNS_PTR, 1u, 1u, &packet, &effective) == RATOS_OK);
    assert(strstr(effective, "ip6.arpa") != NULL); free(packet.data); free(effective); ratos_context_destroy(ctx);
}

static void test_record_types(void) {
    const uint8_t ipv4[] = {192, 0, 2, 1};
    const uint8_t ipv6[] = {0x20,1,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,1};
    const uint8_t target[] = {6,'t','a','r','g','e','t',7,'e','x','a','m','p','l','e',0};
    uint8_t mx[sizeof(target) + 2u], srv[sizeof(target) + 6u], txt[] = {5,'h','e','l','l','o',5,'w','o','r','l','d'};
    uint8_t caa[] = {0,5,'i','s','s','u','e','c','a','.','e','x','a','m','p','l','e'};
    uint8_t hinfo[] = {6,'x','8','6','_','6','4',4,'U','N','I','X'};
    uint8_t wks[] = {192,0,2,4,6,0x80};
    uint8_t naptr[128]; packet_writer nw = {{0}, 0u};
    uint8_t soa[128]; packet_writer sw = {{0}, 0u};
    expect_text(RATOS_DNS_A, RATOS_DNS_A, ipv4, sizeof(ipv4), "192.0.2.1");
    expect_text(RATOS_DNS_AAAA, RATOS_DNS_AAAA, ipv6, sizeof(ipv6), "2001:db8::1");
    expect_text(RATOS_DNS_CNAME, RATOS_DNS_CNAME, target, sizeof(target), "target.example");
    expect_text(RATOS_DNS_NS, RATOS_DNS_NS, target, sizeof(target), "target.example");
    expect_text(RATOS_DNS_PTR, RATOS_DNS_PTR, target, sizeof(target), "target.example");
    mx[0] = 0; mx[1] = 10; memcpy(mx + 2, target, sizeof(target));
    expect_text(RATOS_DNS_MX, RATOS_DNS_MX, mx, sizeof(mx), "10 target.example");
    expect_text(RATOS_DNS_TXT, RATOS_DNS_TXT, txt, sizeof(txt), "\"hello\" \"world\"");
    expect_text(RATOS_DNS_HINFO, RATOS_DNS_HINFO, hinfo, sizeof(hinfo), "\"x86_64\" \"UNIX\"");
    expect_text(RATOS_DNS_WKS, RATOS_DNS_WKS, wks, sizeof(wks), "\\# 6 c00002040680");
    srv[0]=0; srv[1]=1; srv[2]=0; srv[3]=2; srv[4]=1; srv[5]=187; memcpy(srv+6,target,sizeof(target));
    expect_text(RATOS_DNS_SRV, RATOS_DNS_SRV, srv, sizeof(srv), "1 2 443 target.example");
    expect_text(RATOS_DNS_CAA, RATOS_DNS_CAA, caa, sizeof(caa), "0 issue \"ca.example\"");
    u16(&nw, 10u); u16(&nw, 20u);
    nw.data[nw.length++]=1; nw.data[nw.length++]='s';
    nw.data[nw.length++]=7; bytes(&nw,(const uint8_t *)"SIP+D2U",7);
    nw.data[nw.length++]=0; name(&nw,"target.example");
    memcpy(naptr,nw.data,nw.length);
    expect_text(RATOS_DNS_NAPTR, RATOS_DNS_NAPTR, naptr, nw.length,
        "10 20 \"s\" \"SIP+D2U\" \"\" target.example");
    name(&sw, "ns.example"); name(&sw, "hostmaster.example"); u32(&sw,1);u32(&sw,2);u32(&sw,3);u32(&sw,4);u32(&sw,5);
    memcpy(soa, sw.data, sw.length); expect_text(RATOS_DNS_SOA, RATOS_DNS_SOA, soa, sw.length, "ns.example hostmaster.example 1 2 3 4 5");
    expect_text(RATOS_DNS_A, 65000u, ipv4, sizeof(ipv4), "\\# 4 c0000201");
}

static void test_malformed(void) {
    ratos_context *ctx = ratos_context_create(); ratos_dns_result *result = NULL;
    uint8_t loop[] = {0x12,0x34,0x81,0x80,0,1,0,0,0,0,0,0,0xc0,0x0c,0,1,0,1};
    uint8_t short_packet[] = {0,1,2};
    assert(ratos_dns_parse_response(ctx, loop, sizeof(loop), 0x1234u, "example.com", RATOS_DNS_A, "fixture", &result) == RATOS_ERROR_PROTOCOL);
    assert(ratos_dns_parse_response(ctx, short_packet, sizeof(short_packet), 1u, "example.com", RATOS_DNS_A, "fixture", &result) == RATOS_ERROR_PROTOCOL);
    ratos_context_destroy(ctx);
}

int main(void) {
    assert(ratos_abi_version() == RATOS_ABI_VERSION);
    test_builder(); test_record_types(); test_malformed();
    puts("Ratatoskr DNS native tests passed"); return 0;
}
