#include <stdint.h>
#include "ratatoskr/version.h"

uint32_t ratos_abi_version(void) { return RATOS_ABI_VERSION; }
uint32_t ratos_version_major(void) { return RATOS_VERSION_MAJOR; }
uint32_t ratos_version_minor(void) { return RATOS_VERSION_MINOR; }
uint32_t ratos_version_patch(void) { return RATOS_VERSION_PATCH; }
