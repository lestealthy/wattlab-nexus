# WattLab Nexus Architecture

## Overview

WattLab Nexus uses a layered, modular architecture designed for embedded systems
with real-time constraints. The system is built on FreeRTOS with Arduino APIs as
the primary development interface, and falls back to CMSIS/HAL where timing or
bandwidth demands it.

## Implemented Modules

> The table below reflects what actually exists in `firmware/src` today. It is
> kept in sync with the code rather than describing aspirational features.

| Module | Header | Responsibility |
|--------|--------|----------------|
| errors | `nexus_errors.h` | Structured error codes + strings |
| log | `nexus_log.h` | Levelled, module-tagged logging (+ structured sink) |
| logger | `nexus_logger.h` | Non-blocking persistent log queue, rotation, JSONL/TXT |
| config | `nexus_config.h` | Versioned, checksummed configuration |
| calibration | `nexus_calibration.h` | Versioned ADC/voltage/current calibration |
| time | `nexus_time.h` | Authoritative UTC wall clock + formatting |
| storage | `nexus_storage.h` | Storage facade, backends, path safety, atomic writes |
| persist | `nexus_persist.h` | Capture + test-report persistence formats |
| boot | `nexus_boot.h` | Boot-time crash persistence |
| platform | `nexus_platform.h` | Reset reason + RTC/TimeService init |
| resource | `nexus_resource.h` | Pin/peripheral ownership manager |
| board | `nexus_board.h` | Verified pin definitions |
| capture | `nexus_capture.h` | Ring buffer + trigger engine |
| decoders | `nexus_decoders.h` | Pure UART/I2C/SPI/CAN decoders |
| uart/i2c/spi/can/gpio | `nexus_*.h` | Protocol engine interfaces |
| device_db | `nexus_device_db.h` | Extensible device database |
| discovery | `nexus_discovery.h` | Bus scan + candidate identification |
| test_engine | `nexus_test_engine.h` | JSON test parse/validate/execute |
| measurement | `nexus_measurement.h` | ADC/frequency/duty/RMS/statistics |
| power | `nexus_power.h` | DUT power status + protection flags |
| diagnostics | `nexus_diagnostics.h` | Self-test (RTC/SD aware) + performance info |
| crash | `nexus_crash.h` | Fault record, reset reason, report |
| message | `nexus_message.h` | Internal framed binary protocol |
| ui / ui_lvgl | `nexus_ui.h`, `nexus_ui_lvgl.h` | Navigation + display scaffold |

Device-only translation units (guarded by `ARDUINO_ARCH_STM32`): `nexus_storage_sd.cpp`,
`nexus_rtc_stm32.cpp`, `nexus_platform_stm32.cpp`.

## Verification Status

- Host unit/integration tests: **14 suites, passing** (`test-results/`),
  including storage, time, logger and an end-to-end Alpha flow.
- Firmware: compiles for `STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG`
  (68,136 B flash / 44,000 B RAM at Alpha).
- SD, RTC, LCD, SDRAM, QSPI, Ethernet, USB and physical protocol I/O are
  isolated behind interfaces. Hardware-dependent self-test items report
  `NOT TESTABLE` / `NOT PRESENT` / `NOT VERIFIED`; see
  `docs/ALPHA_HARDWARE_RESULTS.md`.

## Layer Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐ ┌─────────────────┐  │
│  │   UI    │ │ Screens │ │   Nav    │ │  Notifications  │  │
│  └─────────┘ └─────────┘ └──────────┘ └─────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Services Layer                            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │ Discovery│ │   Test   │ │ Capture  │ │    Logger     │  │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │ Protocol │ │  Device  │ │Calibratn │ │  Diagnostics  │  │
│  │ Analyzer │ │   DB     │ │          │ │               │  │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Protocols Layer                           │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐  │
│  │ UART │ │ I2C  │ │ SPI  │ │ CAN  │ │ GPIO │ │ 1-Wire │  │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └────────┘  │
├─────────────────────────────────────────────────────────────┤
│                 Hardware Abstraction Layer                   │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐  │
│  │ GPIO │ │ DMA  │ │Timers│ │ ADC  │ │ UART │ │  I2C   │  │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └────────┘  │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐  │
│  │ SPI  │ │ CAN  │ │  SD  │ │ QSPI │ │SDRAM │ │  LCD   │  │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     Platform Layer                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Arduino   │ │    STM32    │ │      FreeRTOS       │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## FreeRTOS Task Architecture

| Task | Priority | Stack | Description |
|------|----------|-------|-------------|
| Health | 5 | 2KB | System monitoring, watchdog |
| Capture | 4 | 4KB | Data acquisition |
| UI | 3 | 8KB | Display rendering, touch |
| Protocol | 3 | 4KB | Protocol engine processing |
| System | 2 | 4KB | Initialization, clock advance |
| Test | 2 | 4KB | Automated test execution |
| Storage | 1 | 4KB | Queued writes / log flush |
| Logger | 1 | 2KB | Periodic log flush backup |

Data-path priority (highest first): ISR/DMA > Capture > Protocol > Measurement >
Test > UI > Storage > Background. Storage is deliberately low priority so SD
writes never stall capture/UI.

## Persistent Data Pipeline

```
                RTC
                 |
                 v
            TimeService  (single wall clock, UTC)
                 |
   +--------+----+-----+---------+
   v        v          v         v
 Logger  Capture     Tests      UI
   |        |          |         |
   +--------+----+-----+---------+
                 v
           StorageService
                 |
                 v
                SD
```

Monotonic timing (timeouts, durations, capture timing) stays on FreeRTOS/
hardware timers and is never mixed with the wall clock. See `TIME.md`.

Storage writes never happen in UI, capture, protocol or ISR context. Records
are queued and drained by the storage task (see `STORAGE.md`).

## Memory Map

| Region | Size | Usage |
|--------|------|-------|
| Internal SRAM | 320 KB | Task stacks, DMA buffers, protocol state |
| External SDRAM | 8 MB | Waveform buffers, frame buffers, history |
| QSPI Flash | 16 MB | Application assets, configuration, fonts |
| microSD | User | Test reports, captures, logs, exports |

## Communication Protocol

Internal communication uses structured messages:

```
┌──────────┬──────────┬───────────┬─────────────┬──────────┐
│ Version  │ Msg Type │ Timestamp │ Payload Len │ Payload  │
│ 1 byte   │ 1 byte   │ 4 bytes   │ 2 bytes     │ N bytes  │
└──────────┴──────────┴───────────┴─────────────┴──────────┘
```

## Design Principles

1. **Layered Architecture**: Clear separation between UI, services, protocols, and hardware
2. **Hardware Abstraction**: Arduino APIs by default, direct register access when justified
3. **Resource Management**: Centralized pin and peripheral ownership tracking
4. **Deterministic Behavior**: No dynamic allocation in real-time paths
5. **Defensive Design**: All inputs validated, all errors handled
6. **Testability**: Host-side simulation and unit testing for all logic
