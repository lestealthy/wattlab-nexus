# WattLab Nexus 0.2.0-beta.1 — Release Note

**Release type:** Beta (hardware-integrated, pending physical validation)
**Date:** 2026-10-03
**Target:** STM32F746G-DISCO (STM32F746NGH6)
**Previous release:** 0.1.0 (Alpha, software-verified)

## Summary

This release turns WattLab Nexus from a software-verified foundation into a
hardware-integrated firmware with a real persistent-data lifecycle. It adds a
storage service, an authoritative time service, structured logging, capture and
report persistence, and boot-time crash persistence — all behind interfaces
that run identically on the host and on the target.

**It is called Beta, not stable, because no physical STM32F746G-DISCO has been
attached yet.** SD and RTC integration compile and are exercised on the host
through a real filesystem backend, but they are not verified on silicon.

## Highlights

1. **One storage facade, two backends.** Application code never touches the SD
   library. The SD backend (SDMMC1 + FatFs) and the host backend implement the
   same vtable, so storage logic is unit-tested without hardware.
2. **Atomic, power-loss-aware writes.** `write_atomic()` uses `.tmp` → rename;
   leftover temp files are recovered at mount.
3. **One authoritative wall clock.** `TimeService` is the only source of time
   for logs, filenames, reports and the UI. Monotonic timing stays separate.
4. **Non-blocking logging.** `NEXUS_LOG_*` enqueues; a low-priority task drains
   to SD with a documented drop policy and rotation.
5. **Honest hardware reporting.** The self-test distinguishes
   `PASS`/`FAIL`/`NOT_PRESENT`/`NOT_TESTABLE` and never claims a hardware pass.

## Build metrics

| Metric | 0.1.0 | 0.2.0-beta.1 |
|--------|-------|--------------|
| Flash | 38,868 B (3%) | 68,136 B (6%) |
| RAM (static) | 27,980 B (8%) | 44,000 B (13%) |
| Host test suites | 10 | 14 |
| Compiler warnings | 0 | 0 |

## Verification performed

- Host tests: **14/14 suites pass** (0 warnings with `-Wall -Wextra`).
- Simulator: full lifecycle (RTC → storage → discovery → capture → report →
  logs → eject) passes.
- Firmware: compiles for `STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG`
  with `--warnings all` and no warnings.
- Static analysis: pass.

## Not verified (requires hardware)

- microSD detection, mount and I/O on a real card.
- RTC read/write on silicon.
- RTC persistence across power removal — **not possible on this board** (no LSE
  crystal, no VBAT; RTC runs from LSI).
- LCD, touch, SDRAM, QSPI, Ethernet, USB, physical protocol I/O.

See `docs/ALPHA_HARDWARE_RESULTS.md`.

## Upgrade notes

- The canonical version is now `0.2.0-beta.1` in `firmware/include/nexus_version.h`.
- New Arduino libraries are required: `STM32duino STM32SD` and `FatFs`
  (`scripts/setup.bat` installs them).
- `nexus_storage.h` is a breaking change from 0.1.0's placeholder API; all
  callers use the new facade.

## Quick start

```
scripts\setup.bat
scripts\test-all.bat
scripts\build.bat
```

## Full changelog

See [`CHANGELOG.md`](../CHANGELOG.md).
