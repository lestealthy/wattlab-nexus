#ifndef NEXUS_TIME_H
#define NEXUS_TIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * TimeService - the single authoritative application wall-clock.
 *
 * Design:
 *   - Wall-clock time is kept as Unix epoch milliseconds (UTC).
 *   - A monotonic source (millis on the device, an injectable counter on the
 *     host) advances the wall clock between RTC reads.
 *   - The RTC is optional and reached only through nexus_rtc_backend_t.
 *   - If no valid time has ever been established, the state is reported as
 *     INVALID. Nexus never fabricates a plausible-looking date.
 *
 * Monotonic vs wall clock:
 *   - This module owns WALL time (logs, filenames, reports, UI).
 *   - Timeouts, durations, protocol and capture timing MUST use a monotonic
 *     clock (millis/micros, FreeRTOS ticks), never this module.
 */

typedef enum {
    NEXUS_TIME_INVALID = 0,        // no valid time ever established
    NEXUS_TIME_NOT_INITIALIZED,    // service not started
    NEXUS_TIME_VALID,              // wall clock is usable
    NEXUS_TIME_CLOCK_LOST,         // RTC backup domain lost; time untrusted
} nexus_time_state_t;

typedef struct {
    int16_t year;    // e.g. 2026
    uint8_t month;   // 1..12
    uint8_t day;     // 1..31
    uint8_t hour;    // 0..23
    uint8_t minute;  // 0..59
    uint8_t second;  // 0..59
    uint16_t ms;     // 0..999
} nexus_datetime_t;

// ---- RTC backend -----------------------------------------------------------
typedef struct {
    // Returns true if readable and the readable value is trusted.
    bool (*read)(nexus_datetime_t* out);
    // Write a new wall-clock value. Returns true on success.
    bool (*write)(const nexus_datetime_t* in);
    // Hardware present at all?
    bool (*present)(void);
} nexus_rtc_backend_t;

// ---- Monotonic injection (tests / device glue) -----------------------------
typedef uint32_t (*nexus_time_monotonic_fn)(void);

// ---- Lifecycle -------------------------------------------------------------
nexus_err_t nexus_time_init(void);
void nexus_time_set_rtc(nexus_rtc_backend_t* backend);
void nexus_time_set_monotonic(nexus_time_monotonic_fn fn);

// Advance the service. Call periodically (e.g. once per second) with the
// current monotonic millisecond counter. Safe to call at any rate.
void nexus_time_update(uint32_t monotonic_ms);

// Re-read the RTC and adopt it if valid.
nexus_err_t nexus_time_sync_from_rtc(void);

// ---- Queries ---------------------------------------------------------------
nexus_time_state_t nexus_time_state(void);
bool nexus_time_is_valid(void);
uint64_t nexus_time_unix_ms(void);
uint64_t nexus_time_unix_seconds(void);
nexus_err_t nexus_time_now(nexus_datetime_t* out);

// ---- Setting ---------------------------------------------------------------
nexus_err_t nexus_time_set_unix_ms(uint64_t unix_ms);
nexus_err_t nexus_time_set_datetime(const nexus_datetime_t* dt);
// Mark the clock as lost (e.g. RTC backup domain failure detected at boot).
void nexus_time_mark_lost(void);

// ---- Formatting ------------------------------------------------------------
// All write a NUL-terminated string; return NEXUS_OK or NEXUS_ERR_INVALID_PARAM.
nexus_err_t nexus_time_format_iso8601(const nexus_datetime_t* dt, char* out, size_t out_size);
nexus_err_t nexus_time_format_date(const nexus_datetime_t* dt, char* out, size_t out_size);  // YYYY-MM-DD
nexus_err_t nexus_time_format_time(const nexus_datetime_t* dt, char* out, size_t out_size);  // HH:MM:SS
nexus_err_t nexus_time_format_clock(const nexus_datetime_t* dt, char* out, size_t out_size); // HH:MM:SS.mmm
// Compact filename stamp: YYYYMMDD_HHMMSS
nexus_err_t nexus_time_format_stamp(const nexus_datetime_t* dt, char* out, size_t out_size);

// ---- Calendar conversion (pure, testable) ----------------------------------
bool nexus_time_is_leap_year(int year);
uint8_t nexus_time_days_in_month(int year, uint8_t month);
// Converts between Unix seconds and a broken-down UTC datetime.
int64_t nexus_time_to_unix_seconds(const nexus_datetime_t* dt);
void nexus_time_from_unix_seconds(int64_t seconds, nexus_datetime_t* out);

#endif
