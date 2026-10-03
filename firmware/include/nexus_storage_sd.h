#ifndef NEXUS_STORAGE_SD_H
#define NEXUS_STORAGE_SD_H

#include "nexus_storage.h"

/*
 * SD card backend for the STM32F746G-DISCO.
 *
 * HARDWARE FACTS (verified against the installed STM32duino core and the
 * upstream STM32F746G-DISCO board description):
 *   - The microSD slot is wired to SDMMC1, 4-bit:
 *       D0=PC8 D1=PC9 D2=PC10 D3=PC11 CK=PC12 CMD=PD2
 *       card-detect = PC13 (active low)
 *   - The core provides HAL_SD_MODULE_ENABLED; the supported library is
 *     "STM32duino STM32SD" (SDIO/SDMMC + FatFs).
 *   - The STM32SD library/hardware uses DMA-capable SDMMC transfers. The
 *     Cortex-M7 D-cache is NOT enabled by the Arduino core's default startup
 *     for this variant, so application buffers are used as-is. If D-cache is
 *     ever enabled, buffers shared with SDMMC DMA must be cache-aligned and
 *     cleaned/invalidated; that is documented in MEMORY_MAP.md.
 *
 * This header is safe to include on the host; the implementation is guarded
 * by ARDUINO_ARCH_STM32.
 */

// Returns the SD backend vtable, or NULL on unsupported builds.
nexus_storage_backend_t* nexus_storage_sd_backend(void);

#endif
