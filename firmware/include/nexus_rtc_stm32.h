#ifndef NEXUS_RTC_STM32_H
#define NEXUS_RTC_STM32_H

#include "nexus_time.h"

/*
 * STM32 RTC backend.
 *
 * HARDWARE FACTS (verified, do not overstate):
 *   - The STM32F746NG has an on-chip RTC in the backup domain.
 *   - The STM32F746G-DISCO board has NO 32.768 kHz LSE crystal by default and
 *     NO battery on VBAT. LSE is therefore unavailable; the RTC is clocked
 *     from LSI (internal ~32 kHz RC), as confirmed by the STM32F746G-DISCO
 *     upstream board description (the RTC is sourced from LSI).
 *   - Consequence: RTC time is NOT preserved across a full power removal.
 *     It is only preserved across a reset while power (VDD) remains applied.
 *     This backend therefore cannot and does not promise persistence.
 *
 * nexus_rtc_stm32_present() reports whether the RTC clock source has actually
 * started and is marching; it does not claim battery-backed persistence.
 */

extern nexus_rtc_backend_t nexus_rtc_stm32_backend;

// Initialise the RTC from its available clock source. Returns NEXUS_OK on
// success. This does not enable any battery-backed persistence (there is none
// on this board).
nexus_err_t nexus_rtc_stm32_init(void);

#endif
