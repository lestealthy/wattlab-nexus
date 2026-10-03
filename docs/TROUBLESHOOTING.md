# Troubleshooting

## Build problems

### `arduino-cli` not found
Install Arduino IDE 2.x or Arduino CLI. `scripts\build.bat` also looks for the
Arduino IDE bundled CLI at
`C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`.

### `Platform 'STM32:stm32' not found`
Add the STM32 board manager URL and install the core:
```
arduino-cli config set board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
arduino-cli core update-index
arduino-cli core install STMicroelectronics:stm32
```

### `STM32FreeRTOS.h` / `STM32SD.h` not found
Install the libraries:
```
arduino-cli lib install "STM32duino FreeRTOS"
arduino-cli lib install "STM32duino STM32SD"
arduino-cli lib install "FatFs"
```
or run `scripts\setup.bat`.

### Build silently targets the wrong MCU (128 KB flash)
The board part number is mandatory. Use
`--fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG`. `scripts\build.bat`
already does this.

### `'C:\Program' is not recognized...`
An older version of the scripts left the Arduino CLI path unquoted. Pull the
latest scripts; they quote it.

## Test problems

### `g++ not found`
Install MinGW-w64: `winget install BrechtSanders.WinLibs.POSIX.UCRT`.

### Tests fail only for storage/time
These suites create temporary directories under `%TEMP%`. Ensure `%TEMP%` is
writable. Delete `%TEMP%\nexus_*` and re-run.

### Warnings in the compile log
The runner compiles with `-Wall -Wextra`. Fix warnings at the source; do not
suppress them. `test-results/compile.log` has the details.

## Runtime / hardware problems

### No serial output
- Confirm the ST-Link Virtual COM Port is selected, 115200 8N1.
- The default `Serial` is USART1 (PB7/PA9) wired to ST-Link.

### SD reported as `NOT PRESENT`
- Ensure a FAT32-formatted card is inserted.
- Card detect is PC13 (active low); a card that is present but unreadable may
  report mount failure rather than not-present.
- A failed mount never formats the card. Retry or inspect it on a PC.

### SD mount fails (`SD FILESYSTEM ERROR`)
Nexus does **not** auto-format. Options: retry, unmount, or run diagnostics.
Formatting (if done) must be performed by you, explicitly, on a PC.

### Time shows `--:--:--`
The wall clock is invalid. This is expected after a full power cycle because the
board has no backup battery and no 32.768 kHz crystal. Set the time to make it
valid.

### Time is lost after power removal — is that a bug?
No. The STM32F746G-DISCO RTC runs from LSI and has no VBAT cell, so time is not
preserved across full power removal. It is preserved across a reset while VDD
remains applied. See `TIME.md`.

### Files not appearing on the card
The logger flushes periodically (default 1 s) from the storage task. Allow a
moment, or trigger an eject/flush. Check the card on a PC after `eject`.

## Getting help

Include the version (`firmware/include/nexus_version.h`), whether a board was
attached, the exact command, and the serial/log output.
