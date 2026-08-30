#ifndef RATATOSKR_ERROR_H
#define RATATOSKR_ERROR_H
#include "ratatoskr/export.h"
typedef enum ratos_error {
    RATOS_OK = 0,
    RATOS_ERROR_GENERIC = 1,
    RATOS_ERROR_INVALID_ARGUMENT = 2,
    RATOS_ERROR_OUT_OF_MEMORY = 3,
    RATOS_ERROR_TIMEOUT = 4,
    RATOS_ERROR_NETWORK = 5,
    RATOS_ERROR_PROTOCOL = 6,
    RATOS_ERROR_DNS = 7,
    RATOS_ERROR_NOT_FOUND = 8,
    RATOS_ERROR_UNSUPPORTED = 9,
    RATOS_ERROR_PERMISSION_DENIED = 10
} ratos_error;
#ifdef __cplusplus
extern "C" {
#endif
RATOS_API const char *ratos_error_string(ratos_error error);
#ifdef __cplusplus
}
#endif
#endif

