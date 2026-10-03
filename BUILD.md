# WattLab Nexus - Build Guide

## Prerequisites

### Required Tools

1. **Arduino CLI** (or Arduino IDE 2.x)
   - Download: https://arduino.github.io/arduino-cli/
   - Or use bundled CLI: `C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`

2. **STM32 Arduino Core**
   - Install via Arduino CLI: `arduino-cli core install STM32:stm32`

3. **C++ Compiler** (for host-side tests)
   - MinGW-w64: `winget install BrechtSanders.WinLibs.POSIX.UCRT`
   - Or any C++17 compatible compiler

### Install the STM32 core and libraries

The STM32 core is not part of the default index. Add the STMicroelectronics
board manager URL, then install the core and the Alpha libraries:

```batch
arduino-cli config set board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
arduino-cli core update-index
arduino-cli core install STMicroelectronics:stm32
arduino-cli lib install "STM32duino FreeRTOS"
arduino-cli lib install "STM32duino STM32SD"
arduino-cli lib install "FatFs"
```

`STM32duino STM32SD` provides SDMMC1 + FatFs for the microSD slot. `FatFs` is
its dependency.

### Verify Installation

```batch
:: Check Arduino CLI
arduino-cli version

:: Check STM32 core
arduino-cli core list

:: Check C++ compiler
g++ --version
```

### Fully automated setup

```batch
scripts\setup.bat
```

## Building

### Firmware Build

```batch
scripts\build.bat
```

This will:
1. Verify Arduino CLI is available
2. Check/install STM32 core
3. Compile firmware for STM32F746G-DISCO
4. Report build status

### Host Tests

```batch
scripts\test.bat
```

or, for machine-readable reports (recommended):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-tests.ps1
```

This will:
1. Compile all unit + integration tests with `-Wall -Wextra`
2. Run all tests
3. Write `test-results\test-results.json` and `.txt`

### Simulator

```batch
scripts\build-simulator.bat
tests\build\simulator.exe
```

or:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-simulator.ps1
```

### Clean

```batch
scripts\clean.bat
```

## Manual Build Commands

### Firmware

The board part number **must** be specified so the STM32F746NGH6 variant
(1 MB flash / 320 KB SRAM) is selected instead of the `Disco` default:

```batch
arduino-cli compile --fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG firmware\wattlab_nexus
```

Building without `pnum=DISCO_F746NG` silently selects a different MCU with
128 KB flash. Nexus therefore pins the correct FQBN in `scripts\build.bat`.

### Host Tests

```batch
g++ -std=c++17 -I firmware\include -I host\mocks -o tests\build\test_runner.exe ^
    tests\unit\test_main.cpp ^
    tests\unit\test_config.cpp ^
    tests\unit\test_capture.cpp ^
    tests\unit\test_device_db.cpp ^
    tests\unit\test_test_engine.cpp ^
    tests\unit\test_resource.cpp ^
    tests\integration\test_integration.cpp ^
    firmware\src\nexus_errors.cpp ^
    firmware\src\nexus_log.cpp ^
    firmware\src\nexus_config.cpp ^
    firmware\src\nexus_resource.cpp ^
    firmware\src\nexus_capture.cpp ^
    firmware\src\nexus_device_db.cpp ^
    firmware\src\nexus_test_engine.cpp
```

## Troubleshooting

### Arduino CLI not found
- Install Arduino IDE 2.x or Arduino CLI
- Add to PATH or use full path

### STM32 core not found
```batch
arduino-cli core update-index
arduino-cli core install STM32:stm32
```

### g++ not found
```batch
winget install BrechtSanders.WinLibs.POSIX.UCRT
```

### Build fails
- Check board FQBN: `arduino-cli board listall | findstr STM32F7`
- Verify all dependencies are installed
- Check compiler version (C++17 required)
