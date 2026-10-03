# WattLab Nexus Alpha - Hardware Results

> **Status: NOT YET PHYSICALLY TESTED.**
>
> No STM32F746G-DISCO was connected during this development phase. The results
> below are deliberately marked `NOT TESTED` / `NOT VERIFIED`. Do not change
> them to `PASS` until the corresponding test in `ALPHA_TEST_PLAN.md` has been
> physically performed and the serial output recorded.

## Environment (to be filled when hardware is available)

| Field | Value |
|-------|-------|
| Date | _(pending)_ |
| Firmware version | 0.1.0 |
| Board revision | _(pending, e.g. MB1190 rev B)_ |
| SD card model | _(pending)_ |
| SD card capacity | _(pending)_ |
| Interface | SDMMC1 4-bit |
| RTC clock source | LSI (no LSE crystal / no VBAT on this board) |

## Results

| Test | Description | Result |
|------|-------------|--------|
| 1 | Boot without SD | NOT TESTED |
| 2 | Boot with SD | NOT TESTED |
| 3 | Time set + persistence | NOT TESTED |
| 4 | Capture save | NOT TESTED |
| 5 | Test report save | NOT TESTED |
| 6 | Hot removal / storage failure | NOT TESTED |
| 7 | Atomic write / power loss | NOT TESTED |
| 8 | Reset reason | NOT TESTED |
| 9 | Crash persistence | NOT TESTED |

## Subsystem status

| Subsystem | Firmware integration | Host simulation | Physical verification |
|-----------|----------------------|-----------------|-----------------------|
| SD storage | Implemented (STM32SD/FatFs) | PASS (host backend) | NOT VERIFIED |
| RTC / TimeService | Implemented (STM32 HAL RTC, LSI) | PASS (mock backend) | NOT VERIFIED |
| Timestamped logging | Implemented (async queue + rotation) | PASS | NOT VERIFIED |
| Capture persistence | Implemented | PASS (e2e) | NOT VERIFIED |
| Report persistence | Implemented | PASS (e2e) | NOT VERIFIED |
| Crash persistence | Implemented (record + boot-time SD write) | PASS (record) | NOT VERIFIED |
| Reset reason | Implemented (RCC flags) | N/A | NOT VERIFIED |

## Notes / corrections

- **RTC persistence:** The F746G-DISCO has no 32.768 kHz LSE crystal and no
  VBAT cell, so the RTC is clocked from LSI and time is **not** preserved across
  full power removal. Test 3 is expected to show the clock lost after power
  removal; this is correct behaviour, not a defect.
- **SD DMA / cache:** D-cache is disabled by default in this core, so SDMMC DMA
  buffers need no cache maintenance today. Re-check before enabling D-cache.

## How to update this file

After each physical test:

1. Record the serial output excerpt.
2. Set the row result to `PASS` / `FAIL` / `NOT PRESENT` / `NOT APPLICABLE`.
3. Add the date, firmware version and any relevant hardware details.
