# WattLab Nexus - Testing Guide

## Philosophy

Every subsystem should have software-only tests that run on a normal desktop,
with no STM32 hardware attached. Hardware-dependent behaviour is exercised
through the host simulator/mocks and clearly reported as
`NOT TESTABLE WITHOUT HARDWARE` where it cannot be verified in software.

## Running Tests

### All host tests (recommended)

```batch
scripts\test.bat
```

or directly:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-tests.ps1
```

This compiles the firmware logic with `-Wall -Wextra`, runs all suites and
writes:

- `test-results\test-results.json` (machine readable)
- `test-results\test-results.txt` (human readable)
- `test-results\test-output.txt` (full console log)
- `test-results\compile.log` (compiler diagnostics)

### Full verification (Alpha definition of done)

```batch
scripts\test-all.bat
```

Runs host tests, the simulator, the firmware build and static analysis, then
prints a consolidated PASS/FAIL summary.

### Simulator

```batch
scripts\build-simulator.bat
```

Runs a virtual DUT (I2C BME280 + MPU6050, UART, GPIO) against a real temporary
filesystem: RTC → storage mount → discovery → capture save → test run → report
save → log flush → clean eject.

## Test Suites

| Suite | What it covers |
|-------|----------------|
| config | init/load/save/validate/reset, checksum tamper detection |
| capture | ring buffer, arm/disarm, trigger detection, overflow |
| device_db | init, lookup by id, identify by register, add/remove |
| test_engine | load/validate/execute/cancel, PASS/FAIL reporting |
| resource | acquire/release/query, owner protection, availability |
| decoders | UART framing, I2C transaction validation, CAN standard frame |
| measurement | ADC calibration, frequency, duty, RMS, statistics |
| message | framing encode/decode, CRC, corruption/truncation rejection |
| crash | record store, checksum tamper detection, formatting, clear |
| time | leap years, unix conversion, boundaries, formatting, RTC states |
| storage | path safety, layout, read/write, atomic write, temp recovery, faults |
| logger | TXT/JSONL format, drop policy, storage failure, rotation |
| integration | full init, discovery, capture+trigger, test engine, resources |
| alpha_flow | end-to-end boot→time→storage→discovery→capture→report→logs→eject |

## Adding Tests

1. Create `tests/unit/test_<name>.cpp` exposing `int test_<name>(void)`
   returning `0` on success and non-zero on failure.
2. Declare it and add it to the table in `tests/unit/test_main.cpp`.
3. Add the test file and any new firmware sources to `scripts/run-tests.ps1`.

## Verification Status

- **Host tests:** run and passing (see `test-results/`).
- **Firmware compile:** verified against
  `STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG`.
- **Hardware behaviour:** not verified — no board attached during development.
  Self-test entries for SDRAM, QSPI, LCD, touch, SD, ADC, DAC, timers, DMA,
  UART, I2C, SPI, CAN, Ethernet and USB return
  `NOT TESTABLE WITHOUT HARDWARE`.

## Hardware Bring-up Checklist (when a board is available)

1. Connect the STM32F746G-DISCO over USB.
2. `arduino-cli upload -p COMx --fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG firmware\wattlab_nexus`
3. Open the serial monitor at 115200 baud (ST-Link VCP, USART1).
4. Confirm the boot banner, module init log and self-test table.
5. Verify the health-task LED (D13) heartbeat.
6. Attach the loopback connector and re-run the self-test.
