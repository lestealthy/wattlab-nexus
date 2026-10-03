/*
 * STM32 platform glue for the Alpha hardware integration.
 *
 * Provides:
 *   - nexus_crash_read_reset_reason(): real STM32 reset flags.
 *   - nexus_rtc_stm32_init() / present probe (declared in nexus_rtc_stm32.h).
 *   - RTC self-test probes for diagnostics.
 *
 * Only built for the STM32 Arduino target.
 */
#if defined(ARDUINO_ARCH_STM32)

#include "nexus_platform.h"
#include "nexus_rtc_stm32.h"
#include "nexus_crash.h"
#include "nexus_diagnostics.h"
#include "nexus_time.h"
#include "nexus_log.h"
#include "stm32_def.h"

// The RTC peripheral is initialised once; track it for the self-test probe.
static bool s_rtc_initialized = false;

// ---- Reset reason ----------------------------------------------------------
uint32_t nexus_crash_read_reset_reason(void) {
    // __HAL_RCC_GET_FLAG returns 1 or 0 for each flag, so test them
    // individually rather than OR-ing into a bitmask.
    bool bor   = __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)  != 0;
    bool pin   = __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)  != 0;
    bool por   = __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)  != 0;
    bool sft   = __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)  != 0;
    bool iwdg  = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0;
    bool wwdg  = __HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0;
    bool lpwr  = __HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != 0;

    uint32_t reason = NEXUS_RESET_UNKNOWN;

    // Order matters: brown-out and watchdog resets are the most specific.
    if (bor)                          reason = NEXUS_RESET_BROWNOUT;
    else if (iwdg)                    reason = NEXUS_RESET_IWDG;
    else if (wwdg)                    reason = NEXUS_RESET_WWDG;
    else if (sft)                     reason = NEXUS_RESET_SOFTWARE;
    else if (lpwr)                    reason = NEXUS_RESET_LOW_POWER;
    else if (pin && por)              reason = NEXUS_RESET_POWER_ON;
    else if (pin)                     reason = NEXUS_RESET_PIN;
    else if (por)                     reason = NEXUS_RESET_POWER_ON;

    // Clear the flags so the next reset is interpreted independently.
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return reason;
}

// ---- RTC probes ------------------------------------------------------------
static bool rtc_probe_available(void) {
    return (RCC->BDCR & RCC_BDCR_RTCEN) != 0;
}
static bool rtc_probe_initialized(void) {
    return s_rtc_initialized;
}

// ---- Initialisation --------------------------------------------------------
// Returns the reset reason captured before the flags were cleared.
uint32_t nexus_platform_init(void) {
    // Record and clear reset reason before anything else.
    uint32_t reason = nexus_crash_read_reset_reason();
    NEXUS_LOG_INFO("PLATFORM", "Reset reason: %s", nexus_crash_reset_reason_string(reason));

    if (nexus_rtc_stm32_init() == NEXUS_OK) {
        s_rtc_initialized = true;
    }

    nexus_time_set_rtc(&nexus_rtc_stm32_backend);
    nexus_diagnostics_set_rtc_probes(rtc_probe_available, rtc_probe_initialized);

    // Adopt the RTC if it holds a plausible value. If it does not, the time
    // service stays INVALID and the UI must show "--:--:--".
    nexus_err_t e = nexus_time_sync_from_rtc();
    if (e == NEXUS_OK) {
        NEXUS_LOG_INFO("PLATFORM", "RTC time valid");
    } else {
        NEXUS_LOG_WARN("PLATFORM", "RTC time unavailable/lost (%d); time is INVALID", e);
        nexus_time_mark_lost();
    }
    return reason;
}

#endif // ARDUINO_ARCH_STM32
