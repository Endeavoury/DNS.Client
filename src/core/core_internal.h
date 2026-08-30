#ifndef RATOS_CORE_INTERNAL_H
#define RATOS_CORE_INTERNAL_H
#include <stddef.h>
#include "ratatoskr/context.h"
#include "ratatoskr/error.h"
struct ratos_context { char error_message[512]; };
void ratos_set_error(ratos_context *ctx, const char *format, ...);
char *ratos_strdup(const char *value);
#endif

