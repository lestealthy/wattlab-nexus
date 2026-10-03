# WattLab Nexus - Memory Map

## Internal SRAM (320 KB)

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| DTCM RAM | 0x20000000 | 128 KB | Task stacks, critical data |
| SRAM1 | 0x20020000 | 112 KB | DMA buffers, protocol state |
| SRAM2 | 0x2004C000 | 16 KB | Heap, general purpose |
| Backup SRAM | 0x40024000 | 4 KB | Crash reports, calibration |

## External SDRAM (8 MB)

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| Frame Buffer | 0xC0000000 | 1.5 MB | LCD frame buffer (double) |
| Waveform Buffer | 0xC0180000 | 2 MB | Capture buffers |
| History Buffer | 0xC0380000 | 1 MB | Protocol history |
| UI Assets | 0xC0480000 | 1 MB | Fonts, icons, images |
| Decoded Packets | 0xC0580000 | 1 MB | Protocol decode storage |
| General Purpose | 0xC0680000 | 1.5 MB | Dynamic allocation |

## QSPI Flash (16 MB)

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| Bootloader | 0x90000000 | 512 KB | Bootloader |
| Application | 0x90080000 | 4 MB | Main firmware |
| Assets | 0x90480000 | 4 MB | Fonts, icons, images |
| Device DB | 0x90880000 | 1 MB | Device database |
| Configuration | 0x90980000 | 512 KB | User configuration |
| Logs | 0x90A00000 | 2 MB | System logs |
| Captures | 0x90C00000 | 4 MB | Captured data |

## microSD Card

| Region | Usage |
|--------|-------|
| /captures | Captured waveforms and protocol data |
| /tests | Test definitions and results |
| /reports | Test reports (JSON, CSV, TXT) |
| /devices | Custom device definitions |
| /config | Configuration files |
| /logs | Exported logs |
| /firmware | Firmware update packages |

## Memory Management Rules

1. **No dynamic allocation in ISRs**: Use static buffers or SDRAM pools
2. **DMA buffers in internal SRAM**: Required for DMA access
3. **Large buffers in SDRAM**: Waveform, history, UI assets
4. **Persistent data in QSPI/SD**: Configuration, logs, captures

## Cache Coherency (Cortex-M7)

The STM32F746 is a Cortex-M7 with a D-cache. The Arduino STM32 core does **not**
enable the D-cache by default for this variant, so buffers shared with DMA
(SDMMC, LCD, ADC) are used as-is today.

**If D-cache is ever enabled**, any DMA-shared buffer must be:

- 32-byte cache-line aligned,
- cleaned (write-back) before a DMA read by the peripheral,
- invalidated after a DMA write into memory, before the CPU reads it.

This applies in particular to SDMMC transfer buffers. Do not enable D-cache
without adding this maintenance. Tracked in `STORAGE.md`.
