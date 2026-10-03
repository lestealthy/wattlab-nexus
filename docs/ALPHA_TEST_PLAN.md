# WattLab Nexus Alpha - Test Plan

Manual procedures for the STM32F746G-DISCO. These require physical hardware.
Software-only results are recorded separately in
`docs/ALPHA_HARDWARE_RESULTS.md`; this file is the procedure.

> Before running any test, flash the current firmware:
> ```
> scripts\build.bat
> arduino-cli upload -p COMx --fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG firmware\wattlab_nexus
> ```
> Open the ST-Link serial monitor at 115200 baud (USART1, PB7/PA9).

The boot banner and self-test table are printed on serial. Record the exact
serial output for each test.

---

## Test 1 - Boot without SD card

**Setup:** No card inserted. Power the board.

**Expected:**
- Nexus boots normally (banner, module init, self-test table).
- `STORAGE SD` line reports `NOT PRESENT` (not FAIL).
- Logger buffers to RAM; it does not crash.
- Health-task LED (D13) blinks.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 2 - Boot with SD card

**Setup:** Insert a FAT32 microSD card. Power/reset.

**Expected:**
- `SD` self-test reports `PASS` (mounted, read OK).
- `/NEXUS` and its standard subdirectories are created on the card.
- A log file appears under `/NEXUS/LOGS/` after the first flush.

**Verify on PC:** remove the card and confirm the directory tree and log file.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 3 - Time

**Setup:** With storage mounted, set the time to `2026-10-03 02:00:00` (via the
UI once implemented, or the platform setter).

**Expected:**
- `RTC` self-test reports valid; note `persistence NOT VERIFIED`.
- UI shows `02:00:xx` (when the UI is present).
- New log timestamps and file names reflect the set date.

**Note:** after a full power removal, expect the clock to be lost (no LSE
crystal / no VBAT). Confirm and record.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 4 - Capture

**Setup:** Generate a demo capture and save it.

**Expected:**
- `/NEXUS/CAPTURES/CAP_<stamp>/meta.json` and `data.bin` exist.
- `meta.json` contains `format`, `version`, `timestamp`, `source`, `protocol`,
  `sample_rate`, `trigger`.
- Data length matches `sample_count` / `data_bytes`.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 5 - Test report

**Setup:** Run the demo/integration test.

**Expected:**
- Result `PASS`.
- `/NEXUS/REPORTS/TEST_<stamp>.json` and `.txt` exist.
- Report includes timestamp, duration and step counts.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 6 - Storage failure

**Setup:** While running, remove the SD card (hot removal).

**Expected:**
- Nexus remains alive; no RTOS crash.
- Storage state becomes `REMOVED`; writes stop and are retained in the RAM
  buffer where possible.
- A warning is shown/logged.
- Re-inserting the card and re-mounting resumes logging.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 7 - Atomic write / power loss

**Setup:** Begin writing a report, cut power mid-write (or use the host fault
injection to confirm behaviour first).

**Expected:**
- On next boot, any leftover `.tmp` file is reported and removed.
- The final file is either complete or absent, never partially valid.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 8 - Reset reason

**Setup:** Trigger a known reset (press the reset button; optionally a
software reset).

**Expected:**
- Boot log reports the real reset reason (`PIN_RESET`, `SOFTWARE`, etc.),
  not `UNKNOWN`, for these cases.

**Result:** `PASS` / `FAIL` / `NOT TESTED`

---

## Test 9 - Crash persistence

**Setup:** Force a crash (e.g. a deliberate fault on a test build).

**Expected:**
- The compact crash record is stored in backup SRAM (fault handler does NOT
  touch the filesystem).
- On the next boot, `/NEXUS/CRASHES/CRASH_<stamp>.json` is written by normal
  firmware, and the serial log reports the previous crash.

**Result:** `PASS` / `FAIL` / `NOT TESTED`
