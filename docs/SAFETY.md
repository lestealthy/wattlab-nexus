# WattLab Nexus - Safety and Defensive Design

## Principles

1. **Never trust configuration or test files.** All JSON inputs are parsed into
   fixed-size structures with explicit bounds; unknown or invalid content is
   rejected rather than partially applied.
2. **No arbitrary test JSON reaches dangerous hardware unchecked.** Test steps
   are a fixed enumeration (`nexus_test_step_t`); there is no "raw command"
   escape hatch.
3. **Power control has explicit boundaries.** DUT power is a separate,
   protected subsystem (`nexus_power`) with over/under-voltage and over-current
   flags. Firmware never drives an arbitrary external DUT supply from an MCU
   pin.
4. **Errors never disappear silently.** Every API returns `nexus_err_t` with a
   human-readable string (`nexus_err_string`).

## Safe software faults vs hardware fault injection

| Category | Examples | Status |
|----------|----------|--------|
| SAFE SOFTWARE FAULTS | UART drop/inject/corrupt byte, I2C NACK modelling, timeout injection, protocol-level malformation | Architecturally supported at the protocol layer |
| HARDWARE FAULT INJECTION | supply glitching, bus contention, short-circuit, brownout injection | **Requires external circuitry and is out of scope for base hardware** |

Software fault injection must never be implemented by abusing MCU pins in a way
that could damage the board or DUT. Where a fault cannot be produced safely in
software, it is expressly listed as requiring external hardware.

## CAN

The STM32F746 integrates bxCAN 2.0B only. Nexus does not claim CAN-FD support;
CAN-FD is modelled as a future external-controller capability behind the same
protocol engine interface.

## Analog measurement

Nexus does not present raw ADC counts as measurements and does not assume
arbitrary analog inputs are safe. Calibration (`nexus_calibration`) is
versioned and checksummed, and voltage/current calibration lives in one place
rather than being scattered as magic constants.

## Crash handling

On a fault, Nexus records a compact, checksummed crash report and reboots
deterministically. It does not attempt unsafe in-place recovery. The next boot
surfaces the report ("LAST SYSTEM CRASH DETECTED").
