# WattLab Nexus - Development Roadmap

Legend: **[x]** implemented and covered by host tests / compile-verified,
**[~]** interface + logic implemented, hardware binding pending,
**[ ]** planned.

## Alpha Phase: Storage, Time & Data Logging

- [x] Storage service facade + backend model
- [x] Host filesystem backend (tests/fault injection)
- [x] SD card backend (SDMMC1 + STM32duino STM32SD/FatFs)
- [x] SD filesystem layout (`/NEXUS/...`) with auto-creation
- [x] Storage state machine (NOT_PRESENT..CORRUPTED)
- [x] Path-safety + name sanitization (traversal-proof)
- [x] Atomic write (`.tmp` -> rename) + temp recovery
- [x] TimeService (Unix ms, UTC, formatting)
- [x] RTC backend (STM32 HAL, LSI-clocked)
- [x] Timestamped structured logging (TXT + JSONL)
- [x] Non-blocking log queue with documented drop policy
- [x] Log rotation (size + retention)
- [x] Capture persistence (`meta.json` + `data.bin`)
- [x] Test report persistence (JSON + TXT)
- [x] Crash persistence (record in backup SRAM, written to SD at boot)
- [x] Real reset-reason detection (RCC flags)
- [x] Hardware-aware self-test (RTC/SD states)
- [x] Host fault-injection tests (absent/mount-fail/write-fail/full)
- [x] End-to-end Alpha flow test
- [~] Physical SD verification (see docs/ALPHA_HARDWARE_RESULTS.md)
- [~] Physical RTC verification (persistence CANNOT be assumed)

## Phase 0: Project Foundation ✅

- [x] Directory structure
- [x] Arduino CLI configuration
- [x] STM32 target configuration (`pnum=DISCO_F746NG`)
- [x] Host test framework (10 suites)
- [x] Logging abstraction
- [x] Configuration system (with checksum validation)
- [x] CI-style scripts (build/test/lint/clean/setup)
- [x] Documentation framework

## Phase 1: Platform ✅

- [x] Arduino startup
- [x] FreeRTOS task architecture (8 tasks)
- [x] Logging
- [~] Memory abstraction (used by capture; SDRAM pooling pending)
- [x] Board definition verified against the installed STM32duino variant
- [x] Resource manager (pin/peripheral ownership)
- [ ] Hardware watchdog (IWDG) binding

## Phase 2: Display/UI 🚧

- [~] UI navigation model + screen enumeration
- [~] LVGL integration scaffold (`nexus_ui_lvgl`)
- [ ] LCD driver (LTDC) hardware binding
- [ ] Touch driver (FT6206) binding
- [ ] Theme system
- [ ] Home / splash / settings screens rendered
- [x] SVG logo variants

## Phase 3: Storage ✅

- [x] Storage abstraction + backend model
- [x] SD card driver (SDMMC1) binding
- [x] Host filesystem backend
- [x] Config/log/capture/report persistence
- [x] Atomic writes + recovery
- [ ] QSPI flash backend

## Phase 4: Protocol Engines 🚧

- [~] UART engine + frame decoder (tests)
- [~] I2C engine + transaction decoder (tests)
- [~] SPI engine
- [~] GPIO engine
- [~] CAN engine + standard-frame decoder (tests)
- [ ] 1-Wire

## Phase 5: Capture Engine 🚧

- [x] Ring buffer
- [x] Trigger detection (rising/falling/threshold)
- [x] Pre/post-trigger configuration
- [x] Overflow handling
- [ ] DMA abstraction binding
- [ ] Waveform timeline rendering

## Phase 6: Device Discovery 🚧

- [x] Extensible device database (6 built-in devices)
- [x] Identity detection by register
- [x] Discovery engine (I2C path) + simulator verification
- [ ] SPI/UART/CAN/GPIO discovery paths

## Phase 7: Measurement 🚧

- [x] ADC conversion + calibration
- [x] Frequency / period
- [x] Duty cycle
- [x] RMS / statistics
- [~] Voltage / power abstraction
- [ ] Pulse width

## Phase 8: Test Engine 🚧

- [x] JSON test schema parsing
- [x] Validation
- [x] Execution engine
- [x] PASS/FAIL/result reporting
- [~] Assertions beyond step-type defaults
- [ ] Report export (JSON/CSV/TXT)

## Phase 9: Self Test / Calibration 🚧

- [x] Diagnostics + self-test (RTC/SD states; other hardware marked NOT TESTABLE)
- [x] Calibration (versioned, checksummed)
- [x] Crash record + report + reset reason
- [x] Boot-time crash persistence to SD
- [ ] Loopback auto-detection
- [ ] Performance monitor

## Phase 10: Advanced Features 📋

- [x] Internal structured message protocol (framing + CRC)
- [ ] Protocol timeline correlation
- [ ] PC companion transport
- [ ] Ethernet / USB
- [ ] HIL abstraction
- [ ] Fault injection abstraction
- [ ] Advanced power profiling

## Future Phases

### Phase 11: Automotive Mode
- [ ] CAN-FD support (external controller)
- [ ] LIN support
- [ ] K-Line support
- [ ] Automotive profiles

### Phase 12: PC Companion
- [ ] Desktop application
- [ ] Live capture viewing
- [ ] Test configuration
- [ ] Report management
- [ ] Firmware updates

### Phase 13: Cloud Integration
- [ ] Remote monitoring
- [ ] Test result sharing
- [ ] Device database sync
- [ ] OTA updates
