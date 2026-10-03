/*
 * STM32 RTC backend implementation.
 *
 * This translation unit only builds for the STM32 Arduino target.
 *
 * Time base note: the F746G-DISCO has no LSE crystal and no VBAT cell, so the
 * RTC runs from LSI. We configure it accordingly and never claim that time
 * survives power removal. See nexus_rtc_stm32.h.
 */
#if defined(ARDUINO_ARCH_STM32)

#include "nexus_rtc_stm32.h"
#include "nexus_log.h"
#include "stm32_def.h"
#include <string.h>

static RTC_HandleTypeDef s_hrtc;

static bool rtc_read(nexus_datetime_t* out) {
    if (!out) return false;
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    if (HAL_RTC_GetTime(&s_hrtc, &t, RTC_FORMAT_BIN) != HAL_OK) return false;
    if (HAL_RTC_GetDate(&s_hrtc, &d, RTC_FORMAT_BIN) != HAL_OK) return false;

    out->year = (int16_t)(2000 + d.Year);
    out->month = d.Month;
    out->day = d.Date;
    out->hour = t.Hours;
    out->minute = t.Minutes;
    out->second = t.Seconds;
    // HAL RTC stores sub-seconds as a fraction of a second.
    out->ms = (uint16_t)(((uint32_t)t.SubSeconds * 1000U) / (t.SecondFraction + 1U));
    return true;
}

static bool rtc_write(const nexus_datetime_t* in) {
    if (!in) return false;

    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    t.Hours = in->hour;
    t.Minutes = in->minute;
    t.Seconds = in->second;
    t.DayLightSaving = RTC_STOREOPERATION_RESET;
    t.StoreOperation = RTC_STOREOPERATION_RESET;

    d.Year = (uint8_t)(in->year % 100);
    d.Month = in->month;
    d.Date = in->day;
    d.WeekDay = RTC_WEEKDAY_MONDAY; // weekday not tracked; placeholder

    if (HAL_RTC_SetTime(&s_hrtc, &t, RTC_FORMAT_BIN) != HAL_OK) return false;
    if (HAL_RTC_SetDate(&s_hrtc, &d, RTC_FORMAT_BIN) != HAL_OK) return false;
    return true;
}

static bool rtc_present(void) {
    // The RTC is present once its clock source is enabled. This does not
    // imply battery backup.
    return (RCC->BDCR & RCC_BDCR_RTCEN) != 0;
}

nexus_rtc_backend_t nexus_rtc_stm32_backend = {
    .read = rtc_read,
    .write = rtc_write,
    .present = rtc_present,
};

// Initialise the RTC from LSI. Returns NEXUS_OK on success.
nexus_err_t nexus_rtc_stm32_init(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_RTC_ENABLE();

    s_hrtc.Instance = RTC;
    s_hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    s_hrtc.Init.AsynchPrediv = 0x7F;   // LSI ~32 kHz -> 256 * 128 = 32768
    s_hrtc.Init.SynchPrediv = 0x00FF;
    s_hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    s_hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    s_hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&s_hrtc) != HAL_OK) {
        NEXUS_LOG_WARN("RTC", "HAL_RTC_Init failed");
        return NEXUS_ERR_HARDWARE;
    }
    return NEXUS_OK;
}

#endif // ARDUINO_ARCH_STM32
