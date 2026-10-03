#include "nexus_time.h"
#include <string.h>
#include <stdio.h>

static bool s_initialized = false;
static nexus_time_state_t s_state = NEXUS_TIME_INVALID;
static nexus_rtc_backend_t* s_rtc = NULL;
static nexus_time_monotonic_fn s_monotonic = NULL;

static uint64_t s_unix_ms = 0;          // wall clock when valid
static uint32_t s_last_mono_ms = 0;     // monotonic value at last update
static bool s_have_mono = false;

// ---- Calendar maths (Howard Hinnant's civil algorithms) --------------------
// days_from_civil: days since 1970-01-01 for a proleptic Gregorian date.
static int64_t days_from_civil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);             // [0, 399]
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;  // [0, 146096]
    return era * 146097 + (int64_t)doe - 719468;
}

// civil_from_days: inverse of days_from_civil.
static void civil_from_days(int64_t z, int* y, unsigned* m, unsigned* d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);           // [0, 146096]
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0,399]
    const int64_t yr = (int64_t)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0,365]
    const unsigned mp = (5 * doy + 2) / 153;                      // [0,11]
    *d = doy - (153 * mp + 2) / 5 + 1;                            // [1,31]
    *m = mp + (mp < 10 ? 3 : -9);                                 // [1,12]
    *y = (int)(yr + (*m <= 2));
}

bool nexus_time_is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t nexus_time_days_in_month(int year, uint8_t month) {
    static const uint8_t d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && nexus_time_is_leap_year(year)) return 29;
    return d[month - 1];
}

int64_t nexus_time_to_unix_seconds(const nexus_datetime_t* dt) {
    if (!dt) return 0;
    int64_t days = days_from_civil(dt->year, dt->month, dt->day);
    return days * 86400 + (int64_t)dt->hour * 3600 +
           (int64_t)dt->minute * 60 + dt->second;
}

void nexus_time_from_unix_seconds(int64_t seconds, nexus_datetime_t* out) {
    if (!out) return;
    int64_t days = seconds / 86400;
    int64_t rem = seconds % 86400;
    if (rem < 0) { rem += 86400; days -= 1; }

    int y; unsigned m, d;
    civil_from_days(days, &y, &m, &d);

    memset(out, 0, sizeof(*out));
    out->year = (int16_t)y;
    out->month = (uint8_t)m;
    out->day = (uint8_t)d;
    out->hour = (uint8_t)(rem / 3600);
    out->minute = (uint8_t)((rem % 3600) / 60);
    out->second = (uint8_t)(rem % 60);
    out->ms = 0;
}

// ---- Lifecycle -------------------------------------------------------------
nexus_err_t nexus_time_init(void) {
    s_initialized = true;
    s_state = NEXUS_TIME_INVALID;
    s_unix_ms = 0;
    s_last_mono_ms = 0;
    s_have_mono = false;
    return NEXUS_OK;
}

void nexus_time_set_rtc(nexus_rtc_backend_t* backend) {
    s_rtc = backend;
}

void nexus_time_set_monotonic(nexus_time_monotonic_fn fn) {
    s_monotonic = fn;
}

nexus_err_t nexus_time_sync_from_rtc(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (!s_rtc || !s_rtc->read) {
        s_state = NEXUS_TIME_INVALID;
        return NEXUS_ERR_NOT_SUPPORTED;
    }
    if (s_rtc->present && !s_rtc->present()) {
        s_state = NEXUS_TIME_INVALID;
        return NEXUS_ERR_HARDWARE;
    }

    nexus_datetime_t dt;
    if (!s_rtc->read(&dt)) {
        s_state = NEXUS_TIME_INVALID;
        return NEXUS_ERR_HARDWARE;
    }

    // Sanity gate: reject obviously bogus RTC values rather than trusting them.
    if (dt.year < 2020 || dt.year > 2100 || dt.month < 1 || dt.month > 12 ||
        dt.day < 1 || dt.day > 31 || dt.hour > 23 || dt.minute > 59 || dt.second > 59) {
        s_state = NEXUS_TIME_CLOCK_LOST;
        return NEXUS_ERR_INVALID_CONFIG;
    }

    s_unix_ms = (uint64_t)(nexus_time_to_unix_seconds(&dt)) * 1000u + dt.ms;
    if (s_monotonic) s_last_mono_ms = s_monotonic();
    s_have_mono = (s_monotonic != NULL);
    s_state = NEXUS_TIME_VALID;
    return NEXUS_OK;
}

void nexus_time_update(uint32_t monotonic_ms) {
    if (!s_initialized) return;
    if (s_state != NEXUS_TIME_VALID) return;
    if (!s_have_mono) {
        s_last_mono_ms = monotonic_ms;
        s_have_mono = true;
        return;
    }
    // Unsigned subtraction handles counter wrap correctly.
    uint32_t delta = monotonic_ms - s_last_mono_ms;
    s_last_mono_ms = monotonic_ms;
    s_unix_ms += delta;
}

// ---- Queries ---------------------------------------------------------------
nexus_time_state_t nexus_time_state(void) { return s_state; }
bool nexus_time_is_valid(void) { return s_state == NEXUS_TIME_VALID; }
uint64_t nexus_time_unix_ms(void) { return s_unix_ms; }
uint64_t nexus_time_unix_seconds(void) { return s_unix_ms / 1000u; }

nexus_err_t nexus_time_now(nexus_datetime_t* out) {
    if (!out) return NEXUS_ERR_INVALID_PARAM;
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (s_state != NEXUS_TIME_VALID) return NEXUS_ERR_INVALID_CONFIG;
    int64_t secs = (int64_t)(s_unix_ms / 1000u);
    nexus_time_from_unix_seconds(secs, out);
    out->ms = (uint16_t)(s_unix_ms % 1000u);
    return NEXUS_OK;
}

// ---- Setting ---------------------------------------------------------------
nexus_err_t nexus_time_set_unix_ms(uint64_t unix_ms) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_unix_ms = unix_ms;
    if (s_monotonic) s_last_mono_ms = s_monotonic();
    s_have_mono = (s_monotonic != NULL);
    s_state = NEXUS_TIME_VALID;

    if (s_rtc && s_rtc->write) {
        nexus_datetime_t dt;
        nexus_time_from_unix_seconds((int64_t)(unix_ms / 1000u), &dt);
        dt.ms = (uint16_t)(unix_ms % 1000u);
        s_rtc->write(&dt);
    }
    return NEXUS_OK;
}

nexus_err_t nexus_time_set_datetime(const nexus_datetime_t* dt) {
    if (!dt) return NEXUS_ERR_INVALID_PARAM;
    if (dt->month < 1 || dt->month > 12 || dt->day < 1 ||
        dt->day > nexus_time_days_in_month(dt->year, dt->month) ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59) {
        return NEXUS_ERR_INVALID_PARAM;
    }
    uint64_t ms = (uint64_t)(nexus_time_to_unix_seconds(dt)) * 1000u + dt->ms;
    return nexus_time_set_unix_ms(ms);
}

void nexus_time_mark_lost(void) {
    s_state = NEXUS_TIME_CLOCK_LOST;
}

// ---- Formatting ------------------------------------------------------------
nexus_err_t nexus_time_format_date(const nexus_datetime_t* dt, char* out, size_t out_size) {
    if (!dt || !out || out_size < 11) return NEXUS_ERR_INVALID_PARAM;
    snprintf(out, out_size, "%04d-%02u-%02u", dt->year, dt->month, dt->day);
    return NEXUS_OK;
}

nexus_err_t nexus_time_format_time(const nexus_datetime_t* dt, char* out, size_t out_size) {
    if (!dt || !out || out_size < 9) return NEXUS_ERR_INVALID_PARAM;
    snprintf(out, out_size, "%02u:%02u:%02u", dt->hour, dt->minute, dt->second);
    return NEXUS_OK;
}

nexus_err_t nexus_time_format_clock(const nexus_datetime_t* dt, char* out, size_t out_size) {
    if (!dt || !out || out_size < 13) return NEXUS_ERR_INVALID_PARAM;
    snprintf(out, out_size, "%02u:%02u:%02u.%03u", dt->hour, dt->minute, dt->second, dt->ms);
    return NEXUS_OK;
}

nexus_err_t nexus_time_format_iso8601(const nexus_datetime_t* dt, char* out, size_t out_size) {
    if (!dt || !out || out_size < 25) return NEXUS_ERR_INVALID_PARAM;
    snprintf(out, out_size, "%04d-%02u-%02uT%02u:%02u:%02u.%03uZ",
             dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second, dt->ms);
    return NEXUS_OK;
}

nexus_err_t nexus_time_format_stamp(const nexus_datetime_t* dt, char* out, size_t out_size) {
    if (!dt || !out || out_size < 16) return NEXUS_ERR_INVALID_PARAM;
    snprintf(out, out_size, "%04d%02u%02u_%02u%02u%02u",
             dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);
    return NEXUS_OK;
}
