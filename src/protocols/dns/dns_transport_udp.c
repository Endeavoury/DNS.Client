#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET ratos_socket;
typedef int ratos_socklen;
#define RATOS_INVALID_SOCKET INVALID_SOCKET
#define ratos_close_socket closesocket
#else
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int ratos_socket;
typedef socklen_t ratos_socklen;
#define RATOS_INVALID_SOCKET (-1)
#define ratos_close_socket close
#endif
#include "protocols/dns/dns_internal.h"

static int socket_start(void) {
#ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return 1;
#endif
}
static void socket_finish(void) {
#ifdef _WIN32
    (void)WSACleanup();
#endif
}
static void set_timeout(ratos_socket socket_handle, uint32_t timeout_ms) {
#ifdef _WIN32
    DWORD value = timeout_ms;
    (void)setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof(value));
    (void)setsockopt(socket_handle, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof(value));
#else
    struct timeval value;
    value.tv_sec = (time_t)(timeout_ms / 1000u);
    value.tv_usec = (suseconds_t)((timeout_ms % 1000u) * 1000u);
    (void)setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
    (void)setsockopt(socket_handle, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));
#endif
}
static int timed_out(void) {
#ifdef _WIN32
    int error = WSAGetLastError();
    return error == WSAETIMEDOUT || error == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT;
#endif
}

ratos_error ratos_dns_udp_exchange(ratos_context *ctx, const char *server,
    uint16_t port, uint32_t timeout_ms, const uint8_t *query, size_t query_length,
    uint8_t **response, size_t *response_length) {
    struct addrinfo hints, *addresses = NULL, *address;
    char service[6];
    ratos_error final_error = RATOS_ERROR_NETWORK;
    if (response == NULL || response_length == NULL) return RATOS_ERROR_INVALID_ARGUMENT;
    *response = NULL; *response_length = 0u;
    if (!socket_start()) { ratos_set_error(ctx, "Unable to initialize socket runtime"); return RATOS_ERROR_NETWORK; }
    memset(&hints, 0, sizeof(hints)); hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP;
    (void)snprintf(service, sizeof(service), "%u", (unsigned)port);
    if (getaddrinfo(server, service, &hints, &addresses) != 0) {
        ratos_set_error(ctx, "Unable to resolve DNS server '%s'", server); socket_finish(); return RATOS_ERROR_NETWORK;
    }
    for (address = addresses; address != NULL; address = address->ai_next) {
        ratos_socket socket_handle = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        uint8_t *buffer;
        int received;
        if (socket_handle == RATOS_INVALID_SOCKET) continue;
        set_timeout(socket_handle, timeout_ms);
        if (connect(socket_handle, address->ai_addr, (ratos_socklen)address->ai_addrlen) != 0) { ratos_close_socket(socket_handle); continue; }
#ifdef _WIN32
        if (send(socket_handle, (const char *)query, (int)query_length, 0) != (int)query_length) {
#else
        if (send(socket_handle, query, query_length, 0) != (ssize_t)query_length) {
#endif
            final_error = timed_out() ? RATOS_ERROR_TIMEOUT : RATOS_ERROR_NETWORK;
            ratos_close_socket(socket_handle); continue;
        }
        buffer = (uint8_t *)malloc(RATOS_DNS_MAX_PACKET);
        if (buffer == NULL) { ratos_close_socket(socket_handle); final_error = RATOS_ERROR_OUT_OF_MEMORY; break; }
#ifdef _WIN32
        received = recv(socket_handle, (char *)buffer, (int)RATOS_DNS_MAX_PACKET, 0);
#else
        received = (int)recv(socket_handle, buffer, RATOS_DNS_MAX_PACKET, 0);
#endif
        if (received > 0) {
            ratos_close_socket(socket_handle); freeaddrinfo(addresses); socket_finish();
            *response = buffer; *response_length = (size_t)received; return RATOS_OK;
        }
        final_error = timed_out() ? RATOS_ERROR_TIMEOUT : RATOS_ERROR_NETWORK;
        free(buffer); ratos_close_socket(socket_handle);
    }
    freeaddrinfo(addresses); socket_finish();
    ratos_set_error(ctx, final_error == RATOS_ERROR_TIMEOUT ? "DNS UDP query timed out" : "DNS UDP exchange failed");
    return final_error;
}
