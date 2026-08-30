#ifndef RATATOSKR_VERSION_H
#define RATATOSKR_VERSION_H
#include <stdint.h>
#include "ratatoskr/export.h"
#define RATOS_ABI_VERSION 1u
#define RATOS_VERSION_MAJOR 0u
#define RATOS_VERSION_MINOR 1u
#define RATOS_VERSION_PATCH 0u
#ifdef __cplusplus
extern "C" {
#endif
RATOS_API uint32_t ratos_abi_version(void);
RATOS_API uint32_t ratos_version_major(void);
RATOS_API uint32_t ratos_version_minor(void);
RATOS_API uint32_t ratos_version_patch(void);
#ifdef __cplusplus
}
#endif
#endif

