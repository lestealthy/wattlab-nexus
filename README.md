# WattLab Nexus

**Universal Embedded Engineering Workstation**

**Version:** `0.2.0-beta.1` (`Beta` — hardware-integrated, pending physical validation)
**Target:** STM32F746G-DISCO (STM32F746NGH6) · **License:** MIT

> **Release status.** The software and hardware integration pass all host tests
> and compile for the target, but no physical board has been used to validate
> SD/RTC yet. Hardware-dependent results are recorded as `NOT VERIFIED` in
> [`docs/ALPHA_HARDWARE_RESULTS.md`](docs/ALPHA_HARDWARE_RESULTS.md). See the
> [changelog](CHANGELOG.md) and [release note](docs/RELEASE_NOTE_0.2.0-beta.1.md).

WattLab Nexus is a portable, professional embedded-engineering workstation based on the STM32F746G-DISCO development board. It combines protocol analysis, device discovery, automated testing, signal generation, and measurement capabilities into a single instrument.

## Features

- **Device Discovery**: Automatically scan and identify connected devices via I2C, SPI, UART, CAN, and GPIO
- **Protocol Analyzers**: UART, I2C, SPI, CAN, GPIO decoding with timeline correlation
- **Capture Engine**: Pre-trigger and post-trigger capture with configurable conditions
- **Test Engine**: JSON-based automated test sequences with PASS/FAIL reporting
- **Signal Generator**: GPIO, PWM, UART, I2C, SPI, CAN generation
- **Power Profiling**: Voltage, current, power, and energy measurement
- **Device Database**: Extensible JSON-based device identification database
- **Persistent Storage**: SD-backed `/NEXUS` tree for logs, captures, reports and crashes
- **Time Service**: Single authoritative UTC wall clock with an RTC backend
- **Structured Logging**: Timestamped TXT/JSONL logs with rotation and a safe drop policy
- **Demo Mode**: Full simulation without hardware for development and demonstration

## Quick Start

### Prerequisites

- [Arduino CLI](https://arduino.github.io/arduino-cli/) or Arduino IDE 2.x
- STM32 Arduino core (`STMicroelectronics:stm32`) + `STM32duino FreeRTOS`
- MinGW-w64 or equivalent C++17 compiler (for host-side tests)

Run `scripts\setup.bat` to check/install these automatically.

### Build Firmware

```batch
scripts\build.bat
```

This targets the STM32F746NG via
`STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG` (1 MB flash / 320 KB SRAM).
See `BUILD.md` for why the part number is mandatory.

### Run Host Tests

```batch
scripts\test.bat
```

Generates `test-results\test-results.json` and `.txt`.

### Run Simulator

```batch
scripts\build-simulator.bat
```

### Developer note: sketch source sync

The Arduino build system only compiles sources inside the sketch folder. The
canonical sources live in `firmware/src` and `firmware/include`;
`scripts\build.bat` calls `scripts\sync-sketch.ps1` to mirror them into
`firmware/wattlab_nexus` before compiling. Edit `src/` / `include/`, never the
mirrored copies.

## Project Structure

```
wattlab-nexus/
├── firmware/               # STM32F746G-DISCO firmware
│   ├── src/               # Canonical source files
│   ├── include/           # Canonical headers
│   └── wattlab_nexus/     # Arduino sketch (mirrors src/ + include/)
├── host/                   # Host-side simulation
│   ├── simulator/         # Virtual DUT simulator
│   └── mocks/             # Hardware mocks for testing
├── tests/                  # Test suites
│   ├── unit/              # Unit tests
│   ├── integration/       # Integration tests
│   └── fixtures/          # Test data
├── docs/                   # Supplemental documentation
├── assets/logo/            # SVG logo variants
├── scripts/                # Build, test, sync and setup scripts
├── configs/                # Configuration files
├── examples/               # Example test definitions
└── test-results/           # Generated test reports
```

## Architecture

The firmware uses a layered architecture:

```
Application (UI, Screens, Navigation)
        ↓
Services (Discovery, Test, Capture, Protocol)
        ↓
Protocols (UART, I2C, SPI, CAN, GPIO)
        ↓
Hardware Abstraction (GPIO, DMA, Timers, ADC)
        ↓
Platform (Arduino, STM32, FreeRTOS)
```

FreeRTOS tasks:
- **System Task**: Initialization and coordination
- **UI Task**: Display rendering and touch handling
- **Capture Task**: Data acquisition
- **Protocol Task**: Protocol engine processing
- **Storage Task**: Queued persistent writes (logs, captures, reports)
- **Test Task**: Automated test execution
- **Logger Task**: Periodic log flush
- **Health Task**: System monitoring and watchdog

## Hardware

- **MCU**: STM32F746NGH6 (Cortex-M7, 216 MHz)
- **Flash**: 1 MB internal
- **SRAM**: ~320 KB internal + external SDRAM
- **Display**: 480×272 RGB LCD-TFT
- **Connectivity**: Ethernet, USB OTG, microSD (SDMMC1)
- **Expansion**: Arduino Uno V3 headers (3.3 V logic)

## Documentation map

| Start here | Then |
|------------|------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | Build and run everything |
| [ARCHITECTURE.md](ARCHITECTURE.md) | How the system is structured |
| [STORAGE.md](STORAGE.md) / [TIME.md](TIME.md) | Persistent data + wall clock |
| [docs/API_REFERENCE.md](docs/API_REFERENCE.md) | Public interfaces |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Extending the code |
| [TESTING.md](TESTING.md) | Test suites and runners |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) / [docs/FAQ.md](docs/FAQ.md) | Common questions |
| [docs/INDEX.md](docs/INDEX.md) | Full documentation index |

## License

MIT License — see the [LICENSE](LICENSE) file.

## Version

Current version: **0.2.0-beta.1** — see [CHANGELOG.md](CHANGELOG.md).
