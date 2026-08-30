#ifndef RATATOSKR_CONTEXT_H
#define RATATOSKR_CONTEXT_H
#include "ratatoskr/export.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ratos_context ratos_context;
RATOS_API ratos_context *ratos_context_create(void);
RATOS_API void ratos_context_destroy(ratos_context *ctx);
/* Borrowed string valid until the next operation on ctx or its destruction. */
RATOS_API const char *ratos_context_error(const ratos_context *ctx);
#ifdef __cplusplus
}
#endif
#endif

