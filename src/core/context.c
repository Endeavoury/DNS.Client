#include <stdlib.h>
#include "core/core_internal.h"

ratos_context *ratos_context_create(void) {
    return (ratos_context *)calloc(1, sizeof(ratos_context));
}

void ratos_context_destroy(ratos_context *ctx) { free(ctx); }

const char *ratos_context_error(const ratos_context *ctx) {
    return (ctx != NULL && ctx->error_message[0] != '\0') ? ctx->error_message : "";
}

