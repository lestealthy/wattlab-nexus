#include "nexus_platform.h"
#include "nexus_crash.h"

// Host / non-STM32 default: no hardware reset-reason source.
// The STM32 implementation in nexus_platform_stm32.cpp overrides this.
__attribute__((weak)) uint32_t nexus_platform_init(void) {
    return NEXUS_RESET_UNKNOWN;
}
