# Changelog

All notable changes to WattLab Nexus are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

During pre-1.0 development, `0.x.y` versions carry pre-release labels
(`-alpha`, `-beta.N`). A build is only labelled stable once the corresponding
hardware validation has been completed (see `docs/ALPHA_TEST_PLAN.md` and
`docs/ALPHA_HARDWARE_RESULTS.md`).

## [Unreleased]

Nothing yet.

## [0.2.0-beta.1] - 2026-10-03

The "hardware-integrated, pending validation" release. Adds persistent storage,
an authoritative time service, structured logging and data persistence behind
testable interfaces. No physical STM32F746G-DISCO validation has been performed
yet; hardware-dependent results are recorded as NOT VERIFIED.

### Added

- **Storage service** (`nexus_storage`)
  - Backend/vtable model with a single facade used by all application code.
  - `SDCardBackend`: SDMMC1 4-bit via STM32duino STM32SD + FatFs; capacity from
    `BSP_SD_GetCardInfo`, free space from `f_getfree`, atomic rename via
    `f_rename`.
  - `HostFilesystemBackend`: real directory tree for tests/simulator with fault
    injection (card full, write failure, mount failure).
  - Explicit state machine (`NOT_PRESENT`…`CORRUPTED`) instead of a boolean.
  - Automatic `/NEXUS` filesystem layout creation.
  - Traversal-proof path validation and filename sanitization.
  - Atomic `.tmp` → rename writes and boot-time temp-file recovery.
  - Quota monitoring with configurable warning/critical/full thresholds.
  - Non-destructive read diagnostics and an explicit opt-in write test.
- **Time service** (`nexus_time`)
  - Single authoritative UTC wall clock stored as Unix milliseconds.
  - Monotonic advance with wrap-safe delta handling.
  - States: `INVALID`, `NOT_INITIALIZED`, `VALID`, `CLOCK_LOST`.
  - Bounds-checked formatting: date, time, clock, ISO-8601, file stamp.
  - Full proleptic-Gregorian calendar conversion.
- **RTC backend** (`nexus_rtc_stm32`, `nexus_platform_stm32`)
  - STM32 HAL RTC initialised from LSI (the board has no LSE crystal / VBAT).
  - Real reset-reason decoding from RCC flags.
  - RTC probes for the self-test.
- **Persistent logging** (`nexus_logger`, extended `nexus_log`)
  - Structured sink hook so `NEXUS_LOG_*` reaches storage without per-call I/O.
  - Non-blocking ring buffer with a documented priority drop policy.
  - TXT and JSON Lines output.
  - Size/retention based rotation.
  - Records retained across transient storage failure for retry.
- **Persistence formats** (`nexus_persist`)
  - Versioned capture format (`meta.json` + `data.bin`).
  - Versioned test reports (JSON + TXT).
- **Crash handling** (extended `nexus_crash`, new `nexus_boot`)
  - JSON crash rendering and human-readable reset reasons.
  - Boot-time crash persistence to `/NEXUS/CRASHES/` (never in the fault
    handler).
- **Diagnostics** (`nexus_diagnostics`)
  - Hardware-aware self-test distinguishing `PASS`, `FAIL`, `NOT_PRESENT`,
  `NOT_TESTABLE` for RTC and SD; other peripherals marked not-yet-integrated.
- **Host test suites**: `test_time`, `test_storage`, `test_logger`,
  `test_alpha_flow` (end-to-end lifecycle).
- **Documentation**: `STORAGE.md`, `TIME.md`, `CHANGELOG.md`, `CONTRIBUTING.md`,
  `SECURITY.md`, `docs/GETTING_STARTED.md`, `docs/DEVELOPMENT.md`,
  `docs/API_REFERENCE.md`, `docs/TROUBLESHOOTING.md`, `docs/FAQ.md`,
  `docs/RELEASE_NOTE_0.2.0-beta.1.md`, `docs/RELEASE_PROCESS.md`,
  `docs/ALPHA_TEST_PLAN.md`, `docs/ALPHA_HARDWARE_RESULTS.md`.

### Changed

- Firmware main loop now follows the Alpha boot order (RTC → storage → logger →
  services → self-test → READY).
- Storage task drains queued writes at low priority so SD stalls cannot block
  capture/UI.
- `nexus_log` forwards every record to a registered structured sink,
  independent of the serial bitmask.
- `nexus_storage.h` reworked from a placeholder stub into the backend model
  above.
- Test/build scripts resolve paths relative to the repository root and quote
  the Arduino CLI path.

### Fixed

- Batch build no longer fails when the Arduino CLI path contains spaces.
- Firmware build pins `pnum=DISCO_F746NG` so the STM32F746NGH6 (1 MB flash)
  variant is always selected.

### Known limitations

- **No physical hardware validation.** SD, RTC, LCD, touch, SDRAM, QSPI,
  Ethernet, USB and physical protocol I/O are unverified on real hardware.
- **RTC time is not preserved across power removal.** The STM32F746G-DISCO has
  no LSE crystal and no VBAT cell; the RTC is LSI-clocked.
- QSPI, LCD and touch drivers are not yet integrated.
- The UI is a navigation/display scaffold; screens are not rendered yet.
- CAN-FD is not supported by the MCU and is modelled only as future external
  hardware.

## [0.1.0] - 2026-10-02

Initial software-verified foundation (Alpha). No hardware integration.

### Added

- Layered architecture with FreeRTOS task model.
- Host test framework, simulator and static analysis scripts.
- Logging, configuration (versioned + checksummed), calibration, resource
  manager.
- Capture ring buffer and trigger engine.
- UART/I2C/SPI/CAN decoders and protocol engine interfaces.
- Extensible device database and discovery engine.
- JSON test engine with PASS/FAIL reporting.
- Measurement subsystem (ADC, frequency, duty, RMS, statistics).
- Internal framed message protocol with CRC-16.
- Compact crash records.
- SVG logo set and project documentation.

[Unreleased]: https://github.com/lestealthy/wattlab-nexus/compare/v0.2.0-beta.1...HEAD
[0.2.0-beta.1]: https://github.com/lestealthy/wattlab-nexus/releases/tag/v0.2.0-beta.1
[0.1.0]: https://github.com/lestealthy/wattlab-nexus
