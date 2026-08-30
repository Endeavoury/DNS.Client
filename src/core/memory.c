#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "core/core_internal.h"

char *ratos_strdup(const char *value) {
    size_t length;
    char *copy;
    if (value == NULL) return NULL;
    length = strlen(value);
    if (length == SIZE_MAX) return NULL;
    copy = (char *)malloc(length + 1u);
    if (copy != NULL) memcpy(copy, value, length + 1u);
    return copy;
}
