# Getting Started

This guide takes you from a fresh checkout to running tests, the simulator and a
firmware build — with no hardware required.

## 1. Prerequisites

- Windows with PowerShell
- [Arduino CLI](https://arduino.github.io/arduino-cli/) or Arduino IDE 2.x
- A C++17 compiler (MinGW-w64)
- Git

`scripts\setup.bat` checks and installs the Arduino CLI, STM32 core,
required libraries (`STM32duino FreeRTOS`, `STM32duino STM32SD`, `FatFs`) and
MinGW-w64.

## 2. Set up

```batch
scripts\setup.bat
```

## 3. Run the software suite (no hardware)

```batch
scripts\test.bat            :: 14 host suites
scripts\build-simulator.bat :: virtual DUT + storage lifecycle
scripts\lint.bat            :: static analysis
```

Or everything at once with a summary:

```batch
scripts\test-all.bat
```

Expected summary:

```
WATTLAB NEXUS TEST SUMMARY
Unit Tests             PASS
Storage Tests          PASS
Time Tests             PASS
Integration Tests      PASS
Simulator              PASS
Firmware Build         PASS
Static Analysis        PASS
TOTAL: PASS
```

## 4. Build the firmware

```batch
scripts\build.bat
```

This targets `STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG` (the
STM32F746NGH6 variant). Building without the part number selects a different MCU
with 128 KB flash — the script pins it for you.

Output (0.2.0-beta.1):

```
Sketch uses 68136 bytes (6%) of program storage space.
Global variables use 44000 bytes (13%) of dynamic memory.
```

## 5. Flash to hardware (optional)

Connect the STM32F746G-DISCO via USB, then:

```batch
arduino-cli upload -p COMx --fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG firmware\wattlab_nexus
```

Open the serial monitor at **115200 8N1** (ST-Link Virtual COM Port, USART1).
You should see the boot banner, module initialisation and the self-test table.

Then follow `ALPHA_TEST_PLAN.md` and record results in
`ALPHA_HARDWARE_RESULTS.md`.

## 6. Understand the project

- `README.md` — overview and layout
- `ARCHITECTURE.md` — layers, modules, data pipeline
- `STORAGE.md` — storage service and filesystem layout
- `TIME.md` — time service and RTC facts
- `docs/DEVELOPMENT.md` — how to extend the code
- `docs/API_REFERENCE.md` — public interfaces
- `docs/TROUBLESHOOTING.md` — common problems

## Where files go on the SD card

At mount, Nexus creates:

```
/NEXUS/{CONFIG,DEVICES,TESTS,REPORTS,CAPTURES,LOGS,CRASHES,EXPORTS,FIRMWARE}
```

Logs land in `/NEXUS/LOGS`, captures in `/NEXUS/CAPTURES`, reports in
`/NEXUS/REPORTS`, crashes in `/NEXUS/CRASHES`.
