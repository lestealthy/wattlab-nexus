#ifndef NEXUS_BOARD_H
#define NEXUS_BOARD_H

#include <stdint.h>

/*
 * STM32F746G-DISCO board pin definitions.
 *
 * AUTHORITATIVE SOURCE
 * --------------------
 * These values are derived from the installed STM32duino core variant:
 *
 *   STMicroelectronics/stm32/3.0.0/variants/STM32F7xx/
 *       F746B(E-G)T_F746N(E-G)H_F750N8H_F756BGT_F756NGH/
 *           variant_DISCO_F746NG.h
 *           variant_DISCO_F746NG.cpp   (digitalPin[] / analogInputPin[])
 *
 * Do NOT trust memory or third-party pinout diagrams. If the core version
 * changes, re-derive these from the variant file named above.
 *
 * Arduino pin numbers used by NexusPins are the *Arduino connector indices*
 * accepted by pinMode()/digitalRead()/digitalWrite()/analogRead(). The STM32
 * port name is recorded in the comment for documentation only.
 *
 * IMPORTANT: the Arduino Uno V3 headers on this board expose 3.3 V logic.
 * They are NOT 5 V tolerant in the "Arduino Uno" sense.
 */

namespace NexusPins {

// --- Arduino digital connector D0..D15 (index -> STM32 pin) ---
constexpr uint8_t D0  = 0;   // PC7
constexpr uint8_t D1  = 1;   // PC6
constexpr uint8_t D2  = 2;   // PG6
constexpr uint8_t D3  = 3;   // PB4
constexpr uint8_t D4  = 4;   // PG7
constexpr uint8_t D5  = 5;   // PI0
constexpr uint8_t D6  = 6;   // PH6
constexpr uint8_t D7  = 7;   // PI3
constexpr uint8_t D8  = 8;   // PI2
constexpr uint8_t D9  = 9;   // PA15
constexpr uint8_t D10 = 10;  // PA8
constexpr uint8_t D11 = 11;  // PB15
constexpr uint8_t D12 = 12;  // PB14
constexpr uint8_t D13 = 13;  // PI1  (also LED_BUILTIN)
constexpr uint8_t D14 = 14;  // PB9  (I2C1 SDA)
constexpr uint8_t D15 = 15;  // PB8  (I2C1 SCL)

// --- Arduino analog connector A0..A5 (index -> STM32 pin) ---
// In the STM32duino core these are digital indices 16..21.
constexpr uint8_t A0 = 16;  // PA0
constexpr uint8_t A1 = 17;  // PF10
constexpr uint8_t A2 = 18;  // PF9
constexpr uint8_t A3 = 19;  // PF8
constexpr uint8_t A4 = 20;  // PF7
constexpr uint8_t A5 = 21;  // PF6

// --- On-board devices ---
// The variant defines LED_GREEN/LED_BUILTIN as a macro; avoid those names.
constexpr uint8_t NEXUS_LED        = 13;  // PI1, active HIGH (Arduino D13)
constexpr uint8_t NEXUS_USER_LED   = 13;  // alias
constexpr uint8_t NEXUS_USER_BUTTON = 22; // PI11, active LOW (variant index 22)

// --- Default bus pins for the Arduino headers (documentation + defaults) ---
// I2C1: SDA = PB9 (D14), SCL = PB8 (D15)
constexpr uint8_t I2C1_SDA = D14;
constexpr uint8_t I2C1_SCL = D15;

// --- ST-Link virtual COM port (Serial) ---
// The core configures SERIAL_UART_INSTANCE 1 (USART1) for the ST-Link VCP:
//   RX = PB7, TX = PA9
constexpr uint8_t STLINK_TX = 24;  // PA9  (variant digital index)
constexpr uint8_t STLINK_RX = 23;  // PB7  (variant digital index)

// --- microSD detect ---
constexpr uint8_t SD_DETECT = 25;  // PC13 (variant digital index, active LOW)

// --- microSD SDMMC1 data/clock/command (NOT Arduino indices) ---
// These are dedicated board nets addressed through the SDMMC HAL, recorded
// here for documentation. The authoritative source is the STM32duino variant
// (PeripheralPins_DISCO_F746NG.c) and the upstream board description.
//   D0 = PC8, D1 = PC9, D2 = PC10, D3 = PC11, CK = PC12, CMD = PD2
//   Card detect = PC13 (active low). Use SD_DETECT_PIN (-D variant macro) with
//   the STM32duino STM32SD library.

// NOTE ON PERIPHERALS NOT ON THE ARDUINO HEADERS:
// LCD (LTDC), SDRAM, QSPI, Ethernet RMII, USB OTG and the SDMMC data lines are
// wired to dedicated board nets, not to the Arduino connectors. They are not
// represented as Arduino pin numbers here; board-specific drivers address them
// through the HAL / CMSIS layer. See docs/HARDWARE.md.

} // namespace NexusPins

#endif
