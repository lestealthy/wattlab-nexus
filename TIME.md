# WattLab Nexus - Time

## Purpose

`TimeService` is the **single authoritative application wall-clock**. There is
exactly one source of time for the logger, captures, reports, filenames and UI.
No subsystem keeps its own clock.

## Two clocks, never mixed

| Clock | Source | Used for |
|-------|--------|----------|
| **Monotonic** | `millis()` / `micros()` / FreeRTOS ticks | timeouts, durations, protocol timing, capture timing, measurements |
| **Wall clock** | `TimeService` (RTC-backed) | logs, filenames, reports, UI timestamps |

Rules:

- Never use wall-clock time for a timeout.
- Never use RTC time to implement a 100 ms delay.
- `nexus_time_update()` advances the wall clock from the monotonic counter
  between RTC reads; it never goes backwards.

## State model

```
NOT_INITIALIZED   service not started
INVALID           no valid time ever established
VALID             wall clock usable
CLOCK_LOST        RTC value untrusted (e.g. backup domain lost)
```

The UI must show `--:--:--` when the state is not `VALID`. Nexus never
fabricates a plausible-looking date. When the wall clock is invalid, log lines
carry a monotonic uptime (`T+123.456`) instead of a fake timestamp.

## Representation and formatting

Internally: **Unix epoch milliseconds (UTC)**, plus a broken-down
`nexus_datetime_t` on demand. No per-timestamp dynamic allocation.

Formatting helpers (all bounds-checked):

- `YYYY-MM-DD` (`nexus_time_format_date`)
- `HH:MM:SS` (`nexus_time_format_time`)
- `HH:MM:SS.mmm` (`nexus_time_format_clock`)
- ISO-8601 UTC, e.g. `2026-10-03T01:23:42.381Z` (`nexus_time_format_iso8601`)
- File stamp `YYYYMMDD_HHMMSS` (`nexus_time_format_stamp`)

Calendar conversion uses Howard Hinnant's civil algorithms, which are correct
across leap years, month and year boundaries (covered by tests).

## Timezone

Firmware timestamps are stored and logged in **UTC**. Display timezone is a
separate UI setting and is not baked into stored data. ISO-8601 strings end in
`Z` to make this explicit.

## RTC backend - hardware facts (verified)

The STM32F746NG has an on-chip RTC in the backup domain. On the
**STM32F746G-DISCO** board, however:

- There is **no 32.768 kHz LSE crystal** populated by default.
- There is **no battery on VBAT**.

Consequently the upstream board description clocks the RTC from **LSI** (the
internal ~32 kHz RC oscillator). See `HARDWARE.md` for the source.

**This means RTC time is NOT preserved across a full power removal.** It is
only preserved across a reset while VDD remains applied. The firmware must not
claim battery-backed persistence, and does not.

### What the self-test reports

- Hardware available / initialized / read OK / validity
- `time valid` -> `RTC PASS` with the note `persistence NOT VERIFIED`
- No clock source -> `NOT PRESENT`
- Backup domain lost -> `CLOCK_LOST`, and the user is asked to set the time

Persistence across power removal cannot be verified in software and is listed
as `NOT VERIFIED` until physical testing confirms the actual board behaviour.

## Backend interface

```c
typedef struct {
    bool (*read)(nexus_datetime_t* out);
    bool (*write)(const nexus_datetime_t* in);
    bool (*present)(void);
} nexus_rtc_backend_t;
```

The STM32 backend lives in `nexus_rtc_stm32.cpp`; a host test backend is
injected in `tests/unit/test_time.cpp`. The TimeService validates RTC values
and rejects implausible dates (year < 2020 or > 2100) rather than trusting
garbage.

## Setting time

`nexus_time_set_datetime()` / `nexus_time_set_unix_ms()` validate the value,
update the wall clock, and write through to the RTC when one is present.
Invalid dates (e.g. Feb 30) are rejected.

## Reset reason

`nexus_crash_read_reset_reason()` reads the real STM32 reset flags (POR, PIN,
software, IWDG, WWDG, low-power, brown-out) and clears them. Unsupported
reasons are never invented; unknown maps to `UNKNOWN`.
