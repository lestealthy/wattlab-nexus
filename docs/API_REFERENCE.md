# API Reference

High-level reference for the public interfaces introduced or stabilised in
0.2.0-beta.1. See the headers for exact signatures and types.

All fallible functions return `nexus_err_t` (0 == `NEXUS_OK`, negative on
error). Use `nexus_err_string(err)` for a human-readable form.

---

## Time — `nexus_time.h`

Single authoritative UTC wall clock.

### Types
- `nexus_time_state_t`: `INVALID`, `NOT_INITIALIZED`, `VALID`, `CLOCK_LOST`
- `nexus_datetime_t`: broken-down UTC date/time with milliseconds
- `nexus_rtc_backend_t`: `{ read, write, present }` function pointers

### Lifecycle
```c
nexus_err_t nexus_time_init(void);
void nexus_time_set_rtc(nexus_rtc_backend_t* backend);
void nexus_time_set_monotonic(nexus_time_monotonic_fn fn);
void nexus_time_update(uint32_t monotonic_ms);   // call ~1/s
nexus_err_t nexus_time_sync_from_rtc(void);
```

### Queries
```c
nexus_time_state_t nexus_time_state(void);
bool nexus_time_is_valid(void);
uint64_t nexus_time_unix_ms(void);
uint64_t nexus_time_unix_seconds(void);
nexus_err_t nexus_time_now(nexus_datetime_t* out);
```

### Setting
```c
nexus_err_t nexus_time_set_unix_ms(uint64_t unix_ms);
nexus_err_t nexus_time_set_datetime(const nexus_datetime_t* dt);
void nexus_time_mark_lost(void);
```

### Formatting (bounds-checked)
```c
nexus_err_t nexus_time_format_date(dt, out, n);      // YYYY-MM-DD
nexus_err_t nexus_time_format_time(dt, out, n);      // HH:MM:SS
nexus_err_t nexus_time_format_clock(dt, out, n);     // HH:MM:SS.mmm
nexus_err_t nexus_time_format_iso8601(dt, out, n);   // ...Z
nexus_err_t nexus_time_format_stamp(dt, out, n);     // YYYYMMDD_HHMMSS
```

### Calendar
```c
bool nexus_time_is_leap_year(int year);
uint8_t nexus_time_days_in_month(int year, uint8_t month);
int64_t nexus_time_to_unix_seconds(const nexus_datetime_t* dt);
void nexus_time_from_unix_seconds(int64_t seconds, nexus_datetime_t* out);
```

---

## Storage — `nexus_storage.h`

Backend-based persistent storage facade.

### State
- `nexus_storage_state_t`: `NOT_PRESENT`, `DETECTING`, `MOUNTING`, `READY`,
  `BUSY`, `ERROR`, `REMOVED`, `CORRUPTED`

### Setup
```c
nexus_err_t nexus_storage_init(void);
nexus_err_t nexus_storage_select(nexus_storage_backend_t* backend);
nexus_err_t nexus_storage_mount(void);
nexus_err_t nexus_storage_unmount(void);
nexus_err_t nexus_storage_eject(void);            // flush + unmount
nexus_storage_state_t nexus_storage_state(void);
bool nexus_storage_is_ready(void);
nexus_err_t nexus_storage_get_info(nexus_storage_info_t* info);
const char* nexus_storage_state_string(nexus_storage_state_t s);
```

### Paths
```c
bool nexus_storage_path_is_safe(const char* path);
nexus_err_t nexus_storage_sanitize_name(const char* name, char* out, size_t n);
```

### Files
```c
bool nexus_storage_exists(const char* path);
nexus_err_t nexus_storage_mkdir(const char* path);
nexus_err_t nexus_storage_remove(const char* path);
nexus_err_t nexus_storage_rename(const char* from, const char* to);
nexus_err_t nexus_storage_read_file(const char* path, uint8_t* buf, size_t n, size_t* read);
nexus_err_t nexus_storage_write_file(const char* path, const uint8_t* data, size_t n);
nexus_err_t nexus_storage_append_file(const char* path, const uint8_t* data, size_t n);
nexus_err_t nexus_storage_write_atomic(const char* path, const uint8_t* data, size_t n);
nexus_err_t nexus_storage_list(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count);
```

### Layout / quota / diagnostics
```c
nexus_err_t nexus_storage_ensure_layout(void);
nexus_err_t nexus_storage_recover_temp_files(void);
uint8_t nexus_storage_used_percent(void);
void nexus_storage_set_thresholds(const nexus_storage_thresholds_t* t);
nexus_err_t nexus_storage_diag_read_test(nexus_storage_diagnostics_t* out);
nexus_err_t nexus_storage_diag_write_test(nexus_storage_diagnostics_t* out);
```

### Backends
- `nexus_storage_sd_backend()` — device (SDMMC1 + STM32SD/FatFs)
- `nexus_storage_host_backend()` — host tests (`host/mocks/nexus_storage_host.h`)

---

## Logging — `nexus_log.h` + `nexus_logger.h`

```c
// nexus_log.h
void nexus_log_init(void);
void nexus_log_set_level(nexus_log_level_t level);
void nexus_log_set_output(uint32_t outputs);
void nexus_log_set_sink(nexus_log_sink_fn sink);
NEXUS_LOG_INFO("MODULE", "format %d", value);   // macros for TRACE..FATAL
```

```c
// nexus_logger.h
void nexus_logger_default_config(nexus_logger_config_t* out);
nexus_err_t nexus_logger_init(const nexus_logger_config_t* cfg);
void nexus_logger_sink(nexus_log_level_t, const char* module, const char* message);
nexus_err_t nexus_logger_flush(uint32_t now_ms);
uint32_t nexus_logger_pending(void);
void nexus_logger_get_stats(nexus_logger_stats_t* out);
nexus_err_t nexus_logger_format_record(const nexus_logger_record_t*, nexus_logger_format_t, char* out, size_t n);
```

Drop policy: TRACE/DEBUG first, then INFO; WARN+ retained.

---

## Persistence — `nexus_persist.h`

```c
nexus_err_t nexus_persist_capture(const nexus_capture_meta_t* meta,
                                  const uint8_t* data, size_t len,
                                  char* out_dir, size_t out_dir_size);
nexus_err_t nexus_persist_capture_meta_json(const nexus_capture_meta_t*, uint32_t data_len, char* out, size_t n);
nexus_err_t nexus_persist_test_report(const char* test_name, int result,
                                      uint32_t duration_ms,
                                      uint32_t passed, uint32_t failed,
                                      uint32_t skipped, uint32_t total,
                                      char* out_json, size_t jn,
                                      char* out_txt, size_t tn);
```

---

## Crash / boot — `nexus_crash.h` + `nexus_boot.h`

```c
nexus_err_t nexus_crash_init(void);
void nexus_crash_record(const char* task_name, uint32_t reset_reason);
bool nexus_crash_has_record(void);
const nexus_crash_record_t* nexus_crash_get_record(void);
nexus_err_t nexus_crash_clear(void);
nexus_err_t nexus_crash_format(char* buf, size_t n);
nexus_err_t nexus_crash_format_json(char* buf, size_t n);
const char* nexus_crash_reset_reason_string(uint32_t reason);
uint32_t nexus_crash_read_reset_reason(void);   // weak; STM32 overrides

nexus_err_t nexus_boot_persist_crash(void);     // write prior crash to SD
```

---

## Platform — `nexus_platform.h`

```c
uint32_t nexus_platform_init(void);   // returns reset reason; weak default UNKNOWN
```

## Diagnostics — `nexus_diagnostics.h`

```c
nexus_err_t nexus_diagnostics_init(void);
nexus_err_t nexus_diagnostics_run_selftest(nexus_selftest_entry_t* results, uint32_t max, uint32_t* count);
void nexus_diagnostics_set_rtc_probes(bool (*available)(void), bool (*initialized)(void));
nexus_err_t nexus_diagnostics_get_info(nexus_diagnostics_t* info);
```

Self-test results include `NOT_PRESENT` and `NOT_TESTABLE` in addition to
`PASS`/`FAIL`/`SKIP`.

---

## Internal protocol — `nexus_message.h`

```c
uint16_t nexus_crc16(const uint8_t* data, size_t len);
nexus_err_t nexus_message_encode(const nexus_message_t* msg, uint8_t* out, size_t out_size, size_t* out_len);
nexus_err_t nexus_message_decode(const uint8_t* in, size_t in_len, nexus_message_t* out, size_t* consumed);
```

Frame: magic(2) ver(1) type(1) payload_len(2) payload(N) crc16(2).
See `COMMUNICATION.md`.
