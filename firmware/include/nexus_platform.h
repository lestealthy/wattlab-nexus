#ifndef NEXUS_PLATFORM_H
#define NEXUS_PLATFORM_H

#include <stdint.h>

/*
 * Platform initialisation hook.
 *
 * On the STM32 target this configures the reset-reason capture, the RTC /
 * TimeService, and the diagnostics probes, then returns the reset reason.
 * On the host it returns NEXUS_RESET_UNKNOWN. Call once at boot.
 */
uint32_t nexus_platform_init(void);

#endif
