#include <stdio.h>
#include <string.h>
#include "nexus_time.h"
#include "nexus_errors.h"

static uint32_t g_fake_mono = 0;
static uint32_t fake_mono(void) { return g_fake_mono; }

static bool g_rtc_ok = true;
static nexus_datetime_t g_rtc_dt = {2026, 10, 3, 1, 23, 42, 0};
static int g_rtc_writes = 0;

static bool rtc_read_impl(nexus_datetime_t* out) {
    if (!g_rtc_ok) return false;
    *out = g_rtc_dt;
    return true;
}
static bool rtc_write_impl(const nexus_datetime_t* in) {
    g_rtc_dt = *in;
    g_rtc_writes++;
    return true;
}
static bool rtc_present_impl(void) { return g_rtc_ok; }

static nexus_rtc_backend_t g_rtc = { rtc_read_impl, rtc_write_impl, rtc_present_impl };

static int test_calendar(void) {
    // Leap year rules
    if (!nexus_time_is_leap_year(2024)) { printf("  FAIL: 2024 not leap\n"); return 1; }
    if (nexus_time_is_leap_year(2023))   { printf("  FAIL: 2023 leap\n"); return 1; }
    if (nexus_time_is_leap_year(1900))   { printf("  FAIL: 1900 leap\n"); return 1; }
    if (!nexus_time_is_leap_year(2000))  { printf("  FAIL: 2000 not leap\n"); return 1; }

    if (nexus_time_days_in_month(2024, 2) != 29) { printf("  FAIL: Feb 2024\n"); return 1; }
    if (nexus_time_days_in_month(2023, 2) != 28) { printf("  FAIL: Feb 2023\n"); return 1; }
    printf("  PASS: leap year / days in month\n");
    return 0;
}

static int test_unix_conversion(void) {
    // 1970-01-01 00:00:00 -> 0
    nexus_datetime_t dt = {1970, 1, 1, 0, 0, 0, 0};
    if (nexus_time_to_unix_seconds(&dt) != 0) { printf("  FAIL: epoch\n"); return 1; }

    // 2000-01-01 00:00:00 -> 946684800
    dt = (nexus_datetime_t){2000, 1, 1, 0, 0, 0, 0};
    if (nexus_time_to_unix_seconds(&dt) != 946684800LL) { printf("  FAIL: 2000\n"); return 1; }

    // 2026-10-03 01:23:42 (UTC) round-trips
    dt = (nexus_datetime_t){2026, 10, 3, 1, 23, 42, 0};
    int64_t secs = nexus_time_to_unix_seconds(&dt);
    nexus_datetime_t back;
    nexus_time_from_unix_seconds(secs, &back);
    if (back.year != 2026 || back.month != 10 || back.day != 3 ||
        back.hour != 1 || back.minute != 23 || back.second != 42) {
        printf("  FAIL: round-trip produced %04d-%02u-%02u %02u:%02u:%02u\n",
               back.year, back.month, back.day, back.hour, back.minute, back.second);
        return 1;
    }
    printf("  PASS: unix conversion round-trip\n");

    // Day rollover: 23:59:59 -> next day 00:00:00
    nexus_datetime_t pre = {2026, 10, 3, 23, 59, 59, 0};
    int64_t p = nexus_time_to_unix_seconds(&pre);
    nexus_datetime_t post;
    nexus_time_from_unix_seconds(p + 1, &post);
    if (post.day != 4 || post.hour != 0 || post.minute != 0 || post.second != 0) {
        printf("  FAIL: day rollover\n"); return 1;
    }
    // Month boundary: Oct 31 -> Nov 1
    nexus_datetime_t oct31 = {2026, 10, 31, 23, 59, 59, 0};
    int64_t o = nexus_time_to_unix_seconds(&oct31);
    nexus_time_from_unix_seconds(o + 1, &post);
    if (post.month != 11 || post.day != 1) { printf("  FAIL: month boundary\n"); return 1; }
    // Year boundary: Dec 31 -> Jan 1
    nexus_datetime_t dec31 = {2026, 12, 31, 23, 59, 59, 0};
    int64_t y = nexus_time_to_unix_seconds(&dec31);
    nexus_time_from_unix_seconds(y + 1, &post);
    if (post.year != 2027 || post.month != 1 || post.day != 1) { printf("  FAIL: year boundary\n"); return 1; }
    printf("  PASS: day/month/year boundaries\n");
    return 0;
}

static int test_formatting(void) {
    nexus_datetime_t dt = {2026, 10, 3, 1, 23, 42, 381};
    char buf[40];

    if (nexus_time_format_date(&dt, buf, sizeof(buf)) != NEXUS_OK || strcmp(buf, "2026-10-03") != 0) {
        printf("  FAIL: date='%s'\n", buf); return 1;
    }
    if (nexus_time_format_time(&dt, buf, sizeof(buf)) != NEXUS_OK || strcmp(buf, "01:23:42") != 0) {
        printf("  FAIL: time='%s'\n", buf); return 1;
    }
    if (nexus_time_format_clock(&dt, buf, sizeof(buf)) != NEXUS_OK || strcmp(buf, "01:23:42.381") != 0) {
        printf("  FAIL: clock='%s'\n", buf); return 1;
    }
    if (nexus_time_format_iso8601(&dt, buf, sizeof(buf)) != NEXUS_OK ||
        strcmp(buf, "2026-10-03T01:23:42.381Z") != 0) {
        printf("  FAIL: iso='%s'\n", buf); return 1;
    }
    if (nexus_time_format_stamp(&dt, buf, sizeof(buf)) != NEXUS_OK ||
        strcmp(buf, "20261003_012342") != 0) {
        printf("  FAIL: stamp='%s'\n", buf); return 1;
    }
    // Undersized buffers must be rejected, not overflowed.
    if (nexus_time_format_iso8601(&dt, buf, 4) != NEXUS_ERR_INVALID_PARAM) {
        printf("  FAIL: small buffer accepted\n"); return 1;
    }
    printf("  PASS: formatting + buffer guards\n");
    return 0;
}

static int test_service_states(void) {
    nexus_time_init();
    nexus_time_set_rtc(&g_rtc);
    nexus_time_set_monotonic(fake_mono);

    // Not initialized -> INVALID until synced
    if (nexus_time_is_valid()) { printf("  FAIL: valid before sync\n"); return 1; }
    if (nexus_time_state() != NEXUS_TIME_INVALID) { printf("  FAIL: state not INVALID\n"); return 1; }

    // Sync from a valid RTC
    g_rtc_ok = true;
    g_rtc_dt = (nexus_datetime_t){2026, 10, 3, 1, 23, 42, 0};
    if (nexus_time_sync_from_rtc() != NEXUS_OK) { printf("  FAIL: sync\n"); return 1; }
    if (!nexus_time_is_valid()) { printf("  FAIL: not valid after sync\n"); return 1; }

    nexus_datetime_t now;
    if (nexus_time_now(&now) != NEXUS_OK) { printf("  FAIL: now\n"); return 1; }
    if (now.year != 2026 || now.month != 10 || now.day != 3) { printf("  FAIL: now date\n"); return 1; }
    printf("  PASS: sync from RTC and now()\n");

    // Advance 5 seconds of monotonic time and confirm wall clock advances.
    uint64_t before = nexus_time_unix_seconds();
    g_fake_mono += 5000;
    nexus_time_update(g_fake_mono);
    uint64_t after = nexus_time_unix_seconds();
    if (after - before != 5) { printf("  FAIL: monotonic advance delta=%llu\n", (unsigned long long)(after - before)); return 1; }
    printf("  PASS: monotonic advance\n");

    // Invalid RTC must not produce a fabricated time.
    g_rtc_ok = false;
    if (nexus_time_sync_from_rtc() == NEXUS_OK) { printf("  FAIL: invalid RTC accepted\n"); return 1; }
    printf("  PASS: invalid RTC rejected\n");

    // Bogus RTC date (clock garbage) must be rejected.
    g_rtc_ok = true;
    g_rtc_dt = (nexus_datetime_t){2000, 1, 1, 0, 0, 0, 0}; // pre-2020: implausible
    nexus_err_t e = nexus_time_sync_from_rtc();
    if (e == NEXUS_OK) { printf("  FAIL: implausible RTC accepted\n"); return 1; }
    printf("  PASS: implausible RTC rejected\n");
    return 0;
}

static int test_set(void) {
    nexus_time_init();
    nexus_time_set_rtc(&g_rtc);
    nexus_time_set_monotonic(fake_mono);
    g_rtc_ok = true;
    g_rtc_writes = 0;

    nexus_datetime_t set = {2026, 10, 3, 2, 0, 0, 0};
    if (nexus_time_set_datetime(&set) != NEXUS_OK) { printf("  FAIL: set datetime\n"); return 1; }
    if (!nexus_time_is_valid()) { printf("  FAIL: not valid after set\n"); return 1; }
    if (g_rtc_writes != 1) { printf("  FAIL: RTC not written (%d)\n", g_rtc_writes); return 1; }

    nexus_datetime_t now;
    nexus_time_now(&now);
    if (now.hour != 2 || now.minute != 0) { printf("  FAIL: set value not applied\n"); return 1; }

    // Invalid datetime is rejected.
    nexus_datetime_t bad = {2026, 2, 30, 0, 0, 0, 0}; // Feb 30 does not exist
    if (nexus_time_set_datetime(&bad) != NEXUS_ERR_INVALID_PARAM) { printf("  FAIL: bad date accepted\n"); return 1; }
    printf("  PASS: set datetime + validation\n");

    // mark_lost transitions state
    nexus_time_mark_lost();
    if (nexus_time_state() != NEXUS_TIME_CLOCK_LOST) { printf("  FAIL: mark_lost\n"); return 1; }
    printf("  PASS: mark_lost\n");
    return 0;
}

int test_time(void) {
    printf("\n");
    if (test_calendar()) return 1;
    if (test_unix_conversion()) return 1;
    if (test_formatting()) return 1;
    if (test_service_states()) return 1;
    if (test_set()) return 1;
    return 0;
}
