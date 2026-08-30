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
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
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
    WSADATA data; return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return 1;
#endif
}
static void socket_finish(void) {
#ifdef _WIN32
    (void)WSACleanup();
#endif
}
static int set_nonblocking(ratos_socket handle, int enabled) {
#ifdef _WIN32
    u_long mode = enabled ? 1u : 0u; return ioctlsocket(handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(handle, F_GETFL, 0); return flags >= 0 && fcntl(handle, F_SETFL, enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) == 0;
#endif
}
static int connect_pending(void) {
#ifdef _WIN32
    int error = WSAGetLastError(); return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
    return errno == EINPROGRESS;
#endif
}
static int wait_connected(ratos_socket handle, uint32_t timeout_ms) {
    fd_set writable;
    struct timeval timeout;
    int socket_error = 0;
#ifdef _WIN32
    int length = (int)sizeof(socket_error);
#else
    socklen_t length = sizeof(socket_error);
#endif
    FD_ZERO(&writable); FD_SET(handle, &writable);
    timeout.tv_sec = (long)(timeout_ms / 1000u); timeout.tv_usec = (long)((timeout_ms % 1000u) * 1000u);
    {
        int selected = select((int)handle + 1, NULL, &writable, NULL, &timeout);
        if (selected == 0) return -1;
        if (selected < 0) return 0;
    }
    return getsockopt(handle, SOL_SOCKET, SO_ERROR, (char *)&socket_error, &length) == 0 && socket_error == 0 ? 1 : 0;
}
static void set_io_timeout(ratos_socket handle, uint32_t timeout_ms) {
#ifdef _WIN32
    DWORD value = timeout_ms;
    (void)setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof(value));
    (void)setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof(value));
#else
    struct timeval value;
    value.tv_sec = (time_t)(timeout_ms / 1000u); value.tv_usec = (suseconds_t)((timeout_ms % 1000u) * 1000u);
    (void)setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
    (void)setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));
#endif
}
static int timed_out(void) {
#ifdef _WIN32
    int value = WSAGetLastError();
    return value == WSAETIMEDOUT || value == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT;
#endif
}
static int send_all(ratos_socket handle, const uint8_t *data, size_t length) {
    size_t sent = 0u;
    while (sent < length) {
#ifdef _WIN32
        int count = send(handle, (const char *)data + sent, (int)(length - sent), 0);
#else
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        ssize_t count = send(handle, data + sent, length - sent, flags);
#endif
        if (count <= 0) return 0;
        sent += (size_t)count;
    }
    return 1;
}
static int receive_all(ratos_socket handle, uint8_t *data, size_t length) {
    size_t received = 0u;
    while (received < length) {
#ifdef _WIN32
        int count = recv(handle, (char *)data + received, (int)(length - received), 0);
#else
        ssize_t count = recv(handle, data + received, length - received, 0);
#endif
        if (count <= 0) return 0;
        received += (size_t)count;
    }
    return 1;
}

ratos_error ratos_dns_tcp_exchange(ratos_context *ctx, const char *server,
    uint16_t port, uint32_t timeout_ms, const uint8_t *query, size_t query_length,
    uint8_t **response, size_t *response_length) {
    struct addrinfo hints, *addresses = NULL, *address;
    char service[6];
    ratos_error error = RATOS_ERROR_NETWORK;
    if (response == NULL || response_length == NULL || query_length > RATOS_DNS_MAX_PACKET) return RATOS_ERROR_INVALID_ARGUMENT;
    *response = NULL; *response_length = 0u;
    if (!socket_start()) return RATOS_ERROR_NETWORK;
    memset(&hints, 0, sizeof(hints)); hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP;
    (void)snprintf(service, sizeof(service), "%u", (unsigned)port);
    if (getaddrinfo(server, service, &hints, &addresses) != 0) { ratos_set_error(ctx, "Unable to resolve DNS server '%s'", server); socket_finish(); return RATOS_ERROR_NETWORK; }
    for (address = addresses; address != NULL; address = address->ai_next) {
        ratos_socket handle = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        uint8_t prefix[2], *buffer;
        size_t length;
        if (handle == RATOS_INVALID_SOCKET || !set_nonblocking(handle, 1)) { if (handle != RATOS_INVALID_SOCKET) ratos_close_socket(handle); continue; }
        if (connect(handle, address->ai_addr, (ratos_socklen)address->ai_addrlen) != 0) {
            int connected;
            if (!connect_pending()) { ratos_close_socket(handle); error = RATOS_ERROR_NETWORK; continue; }
            connected = wait_connected(handle, timeout_ms);
            if (connected != 1) { ratos_close_socket(handle); error = connected < 0 ? RATOS_ERROR_TIMEOUT : RATOS_ERROR_NETWORK; continue; }
        }
        if (!set_nonblocking(handle, 0)) { ratos_close_socket(handle); continue; }
#if !defined(_WIN32) && defined(SO_NOSIGPIPE)
        { int enabled = 1; (void)setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)); }
#endif
        set_io_timeout(handle, timeout_ms);
        prefix[0] = (uint8_t)(query_length >> 8); prefix[1] = (uint8_t)query_length;
        if (!send_all(handle, prefix, 2u) || !send_all(handle, query, query_length) || !receive_all(handle, prefix, 2u)) {
            error = timed_out() ? RATOS_ERROR_TIMEOUT : RATOS_ERROR_NETWORK; ratos_close_socket(handle); continue;
        }
        length = ((size_t)prefix[0] << 8) | prefix[1];
        if (length < RATOS_DNS_HEADER_SIZE) { ratos_close_socket(handle); error = RATOS_ERROR_PROTOCOL; continue; }
        buffer = (uint8_t *)malloc(length);
        if (buffer == NULL) { ratos_close_socket(handle); error = RATOS_ERROR_OUT_OF_MEMORY; break; }
        if (!receive_all(handle, buffer, length)) { error = timed_out() ? RATOS_ERROR_TIMEOUT : RATOS_ERROR_NETWORK; free(buffer); ratos_close_socket(handle); continue; }
        ratos_close_socket(handle); freeaddrinfo(addresses); socket_finish();
        *response = buffer; *response_length = length; return RATOS_OK;
    }
    freeaddrinfo(addresses); socket_finish();
    ratos_set_error(ctx, error == RATOS_ERROR_TIMEOUT ? "DNS TCP connection timed out" : "DNS TCP exchange failed");
    return error;
}
