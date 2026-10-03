# WattLab Nexus - Storage

## Purpose

`StorageService` makes persistent data a first-class subsystem. All application
code (logger, captures, reports, config, device DB) goes through one facade and
never calls the SD library directly. The same facade runs on the host against a
real directory tree, so storage logic is unit-tested without hardware.

## Layering

```
Application (logger, captures, reports, config)
                 |
                 v
        nexus_storage  (facade)
        - path safety
        - state machine
        - routing
                 |
        +--------+---------+
        v                  v
  SDCardBackend      HostFilesystemBackend
  (device, SDMMC)    (host tests / simulator)
```

## State model

Storage is **not** a boolean. `nexus_storage_state_t` is one of:

```
NOT_PRESENT  DETECTING  MOUNTING  READY  BUSY  ERROR  REMOVED  CORRUPTED
```

Operations that require storage check `nexus_storage_is_ready()` and fail
cleanly (`NEXUS_ERR_STORAGE`) when it is not. A missing card never crashes a
FreeRTOS task.

## Filesystem layout

Created automatically at mount; a missing directory is never fatal:

```
/NEXUS
├── CONFIG      system.json, calibration.json
├── DEVICES     external device definitions
├── TESTS       test definitions
├── REPORTS     TEST_<stamp>.json / .txt
├── CAPTURES    CAP_<stamp>/ (meta.json + data.bin)
├── LOGS        <date>.log or <date>.jsonl
├── CRASHES     CRASH_<stamp>.json
├── EXPORTS     user exports, scratch
└── FIRMWARE    update packages
```

## Backends

### SDCardBackend (device)

- Interface: **SDMMC1**, 4-bit.
  - D0=PC8, D1=PC9, D2=PC10, D3=PC11, CK=PC12, CMD=PD2, card-detect=PC13 (active low).
- Library: **STM32duino STM32SD** (SDIO/SDMMC + FatFs). This is the supported
  path in the installed core (`HAL_SD_MODULE_ENABLED`).
- Capacity via `BSP_SD_GetCardInfo`; free space via FatFs `f_getfree`.
- Rename via FatFs `f_rename` (true atomic rename).
- Transfers are DMA-capable through the SDMMC HAL. See the cache note below.

### HostFilesystemBackend (host)

Maps `/NEXUS/...` onto a caller-supplied directory. Implements the full vtable
using the C runtime, including directory listing. Supports fault injection:

- `nexus_storage_host_inject_full(true)`
- `nexus_storage_host_inject_write_failure(true)`
- `nexus_storage_host_inject_mount_failure(true)`

## Path safety

`nexus_storage_path_is_safe()` rejects:

- paths not starting with `/`
- backslashes and control characters
- `..` traversal segments (`/NEXUS/../../etc`)
- empty `//` segments and trailing slashes
- over-length paths

`nexus_storage_sanitize_name()` replaces `/ \ : * ? " < > |` and control
characters with `_`, so filenames can never escape the Nexus root. This is
enforced centrally; callers cannot bypass it.

## Power-loss resilience

- `nexus_storage_write_atomic()` writes `<path>.tmp`, flushes, closes, then
  renames to `<path>` (via `f_rename` on device).
- At mount, `nexus_storage_recover_temp_files()` reports and removes leftover
  `*.tmp` files from interrupted writes.
- Bounded buffers everywhere; no unbounded allocation.

## Cache / DMA note (Cortex-M7)

The STM32F746 is a Cortex-M7. The Arduino STM32 core does **not** enable the
D-cache by default for this variant, so SDMMC DMA buffers are used as-is. If
D-cache is enabled in future, any buffer shared with SDMMC DMA must be
cache-aligned (32 bytes) and cleaned before / invalidated after transfer. This
is recorded in `MEMORY_MAP.md` and must be revisited before enabling D-cache.

## Quota / thresholds

`nexus_storage_used_percent()` reports usage. Default thresholds:

| Level | Percent |
|-------|---------|
| WARNING | 80 |
| CRITICAL | 90 |
| FULL | 95 |

Configurable via `nexus_storage_set_thresholds()`.

## Diagnostics

`nexus_storage_diag_read_test()` is non-destructive (lists the root).
`nexus_storage_diag_write_test()` is **explicit and opt-in** (creates and
removes a probe file). Formatting is never automatic and is not implemented.

## Testing

`tests/unit/test_storage.cpp` covers: path safety, mount + layout, read/write,
append, atomic write, temp recovery, listing, write failure, card full, missing
card, and mount failure. `tests/integration/test_alpha_flow.cpp` exercises the
full lifecycle end to end.
