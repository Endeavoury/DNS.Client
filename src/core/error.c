#include <stdarg.h>
#include <stdio.h>
#include "core/core_internal.h"

const char *ratos_error_string(ratos_error error) {
    switch (error) {
    case RATOS_OK: return "success";
    case RATOS_ERROR_GENERIC: return "general failure";
    case RATOS_ERROR_INVALID_ARGUMENT: return "invalid argument";
    case RATOS_ERROR_OUT_OF_MEMORY: return "out of memory";
    case RATOS_ERROR_TIMEOUT: return "operation timed out";
    case RATOS_ERROR_NETWORK: return "network failure";
    case RATOS_ERROR_PROTOCOL: return "protocol error";
    case RATOS_ERROR_DNS: return "DNS server error";
    case RATOS_ERROR_NOT_FOUND: return "not found";
    case RATOS_ERROR_UNSUPPORTED: return "unsupported operation";
    case RATOS_ERROR_PERMISSION_DENIED: return "permission denied";
    default: return "unknown error";
    }
}

void ratos_set_error(ratos_context *ctx, const char *format, ...) {
    va_list args;
    if (ctx == NULL) return;
    va_start(args, format);
    (void)vsnprintf(ctx->error_message, sizeof(ctx->error_message), format, args);
    va_end(args);
}

