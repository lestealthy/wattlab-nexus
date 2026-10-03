# WattLab Nexus Hardware Reference

> **Source of truth.** All Arduino-connector mappings below are taken from the
> installed STM32duino core variant, **not** from memory:
>
> `STMicroelectronics/stm32/3.0.0/variants/STM32F7xx/F746B(E-G)T_F746N(E-G)H_F750N8H_F756BGT_F756NGH/`
> — `variant_DISCO_F746NG.h` and `variant_DISCO_F746NG.cpp` (`digitalPin[]`,
> `analogInputPin[]`).
>
> If the core is updated, re-verify against those files. The board FQBN part
> number is `pnum=DISCO_F746NG`.

## Target Board

**STM32F746G-DISCO** — Discovery kit with STM32F746NGH6 MCU.

## MCU Specifications

| Parameter | Value |
|-----------|-------|
| MCU | STM32F746NGH6 |
| Core | ARM Cortex-M7 |
| Frequency | 216 MHz (PLL from 25 MHz HSE) |
| Flash | 1 MB |
| SRAM | 320 KB |
| FPU | Double-precision |
| Graphics | Chrom-ART (DMA2D), LTDC |

## Arduino Connector Mapping (3.3 V logic)

Arduino pin numbers below are the values passed to the Arduino APIs. The STM32
port pin is informational.

### Digital

| Arduino | STM32 | Variant index |
|---------|-------|---------------|
| D0 | PC7 | 0 |
| D1 | PC6 | 1 |
| D2 | PG6 | 2 |
| D3 | PB4 | 3 |
| D4 | PG7 | 4 |
| D5 | PI0 | 5 |
| D6 | PH6 | 6 |
| D7 | PI3 | 7 |
| D8 | PI2 | 8 |
| D9 | PA15 | 9 |
| D10 | PA8 | 10 |
| D11 | PB15 | 11 |
| D12 | PB14 | 12 |
| D13 | PI1 | 13 (LED_BUILTIN) |
| D14 | PB9 | 14 (I2C1 SDA) |
| D15 | PB8 | 15 (I2C1 SCL) |

### Analog

| Arduino | STM32 | Variant index |
|---------|-------|---------------|
| A0 | PA0 | 16 |
| A1 | PF10 | 17 |
| A2 | PF9 | 18 |
| A3 | PF8 | 19 |
| A4 | PF7 | 20 |
| A5 | PF6 | 21 |

### Other named pins

| Name | STM32 | Variant index | Notes |
|------|-------|---------------|-------|
| User button | PI11 | 22 | Active LOW |
| LED (green) | PI1 | 13 | Active HIGH, `LED_BUILTIN` |
| ST-Link RX | PB7 | 23 | USART1 |
| ST-Link TX | PA9 | 24 | USART1 |
| SD card detect | PC13 | 25 | |

## Serial / ST-Link

The core sets `SERIAL_UART_INSTANCE = 1` (USART1) as the default `Serial`
port, wired to the on-board ST-Link Virtual COM Port:

- `PIN_SERIAL_RX = PB7`
- `PIN_SERIAL_TX = PA9`

## Onboard Peripherals (not on Arduino headers)

These are wired to dedicated board nets and are addressed through the HAL/CMSIS
layer, not as Arduino pin numbers:

| Peripheral | Interface | Notes |
|------------|-----------|-------|
| LCD 480x272 | LTDC RGB | Framebuffer in external SDRAM |
| Touch | I2C (FT5336/FT6xx6) | Capacitive |
| SDRAM | FMC | 8 MB |
| QSPI Flash | QUADSPI | 16 MB |
| microSD | SDMMC1 (4-bit) | microSD slot |
| Ethernet | RMII | On-board PHY |
| USB OTG | FS/HS | |
| CAN | bxCAN | STM32F746 supports **CAN 2.0B only — not CAN-FD** |
| Quad-SPI flash | QUADSPI | Assets/device DB |

## microSD (SDMMC1)

Verified from the installed STM32duino variant
(`PeripheralPins_DISCO_F746NG.c`) and the upstream STM32F746G-DISCO board
description:

| Signal | Pin |
|--------|-----|
| D0 | PC8 |
| D1 | PC9 |
| D2 | PC10 |
| D3 | PC11 |
| CK | PC12 |
| CMD | PD2 |
| Card detect | PC13 (active low) |

- Interface: **SDMMC1**, 4-bit.
- Supported library: **STM32duino STM32SD** (SDIO/SDMMC + FatFs); the core has
  `HAL_SD_MODULE_ENABLED`.
- Transfers are DMA-capable via the SDMMC HAL. D-cache is off by default in
  this core, so no cache maintenance is required today (see MEMORY_MAP.md).

## RTC / Real-Time Clock

The STM32F746NG has an on-chip RTC in the backup domain. On this board:

- There is **no 32.768 kHz LSE crystal** populated by default.
- There is **no VBAT battery cell**.

Therefore the RTC is clocked from **LSI** (internal ~32 kHz RC). **RTC time is
not preserved across full power removal**; it is only preserved across a reset
while VDD remains applied. The firmware calls it the "RTC / Real-Time Clock"
and documents the persistence mechanism accurately. It never claims
battery-backed persistence.

## CAN

The STM32F746 integrates a **bxCAN 2.0B** controller. It does **not** support
CAN-FD. Physical bus connectivity also requires an external CAN transceiver.
Nexus models CAN as a protocol engine that can later be backed by an external
CAN-FD controller without changing the application layer.

## Power

- Board supplied over USB via ST-Link.
- MCU and Arduino headers: **3.3 V logic**.
- Never connect 5 V logic signals directly to the Arduino headers.

## Pin Conflicts / Reserved Pins

Nexus tracks ownership of pins and peripherals with the resource manager
(`nexus_resource.cpp`). Board-reserved nets (LCD, SDRAM, QSPI, SDMMC, RMII,
USB) must not be claimed by the protocol engines. The header pins D0–D15 and
A0–A5 are the intended expansion surface.

## Building for this board

```batch
arduino-cli compile --fqbn STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG firmware\wattlab_nexus
```

Omitting `pnum=DISCO_F746NG` selects a different MCU with 128 KB flash. Nexus
pins the correct FQBN in `scripts\build.bat`.
