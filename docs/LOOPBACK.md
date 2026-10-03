# WattLab Nexus - Loopback Connector

Loopback tests verify that the MCU's own I/O paths work, independent of any
DUT. They require a small external connector wired to the Arduino headers
(3.3 V logic only).

## Suggested connector

| Test | Wiring | Notes |
|------|--------|-------|
| UART | TX (D1) ↔ RX (D0) | USART3 on the headers |
| SPI | MOSI (D11) ↔ MISO (D12) | Optionally tie CS (D10) |
| GPIO | D2 ↔ D3 | One as output, one as input |
| PWM → input | PWM pin → digital input | Measure frequency/duty |
| I2C | SDA (D14) ↔ SCL (D15) does **not** form a loopback by itself | Use an I2C slave or onboard device |
| DAC → ADC | DAC output → ADC input | Internal, when routed |
| CAN | requires an external transceiver + termination | Not a simple wire loop |

## Behaviour

The self-test engine attempts loopback detection where a passive loopback can
be inferred. If no loopback is present, the affected tests report
`NOT TESTABLE WITHOUT HARDWARE` rather than a false PASS.

> Do not connect 5 V signals. The STM32F746G-DISCO Arduino headers are 3.3 V.
