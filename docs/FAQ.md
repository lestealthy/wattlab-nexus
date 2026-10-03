# FAQ

## General

**What is WattLab Nexus?**
A portable embedded-engineering workstation built on the STM32F746G-DISCO. It
combines device discovery, protocol analysis, capture, measurement, automated
testing and persistent data logging.

**What does "Beta" mean here?**
The software and hardware integration are complete and pass all host tests, but
no physical STM32F746G-DISCO has been used to validate SD/RTC yet. It will be
promoted to a stable label only after the hardware test plan passes.

**Does it need a PC to run?**
No. It is designed to run standalone. A future PC companion is planned but the
firmware does not depend on one.

**Does it need an SD card?**
No. Nexus boots and runs without storage; logging falls back to RAM and
hardware features that require storage report `NOT PRESENT`. A card enables
persistence.

## Storage

**Why can't my code call the SD/FatFs library directly?**
All access goes through `nexus_storage` so the same logic runs on the host for
tests and enforces path safety centrally. Direct calls would break testability
and safety.

**What filesystem does it use?**
FAT via FatFs (STM32duino STM32SD). Use a FAT32 card.

**Does it format my card?**
No, never automatically. Formatting, if needed, is a manual PC operation.

**What happens if I pull the card while it is running?**
Writes stop, storage is marked unavailable, and the application keeps running.
Buffered logs are retained where possible and resume when the card returns.
Nexus never crashes because the card disappeared.

**What is the `.tmp` file I see?**
An interrupted atomic write. On the next mount Nexus reports and removes leftover
`.tmp` files. Final files are either complete or absent.

**Where do files go?**
Under `/NEXUS` on the card: `LOGS`, `CAPTURES`, `REPORTS`, `CRASHES`, plus
`CONFIG`, `DEVICES`, `TESTS`, `EXPORTS`, `FIRMWARE`.

## Time / RTC

**Does the clock keep time when powered off?**
No. The F746G-DISCO has no 32.768 kHz crystal and no VBAT battery, so the RTC
runs from the internal LSI and does not survive full power removal.

**Why not add an RTC battery?**
That is a hardware modification. Nexus documents the limitation accurately and
does not claim persistence it does not have.

**What timezone are logs in?**
UTC, with an explicit `Z` in ISO-8601 timestamps. Display timezone is a
separate UI setting.

**Why does an invalid clock show `T+123.456` in logs?**
To avoid fabricating a date. When the wall clock is invalid, log lines carry
monotonic uptime instead of a fake timestamp.

## Testing

**How many tests are there?**
14 host suites, run with `scripts\test.bat`, plus the simulator and firmware
build via `scripts\test-all.bat`.

**Can I test without hardware?**
Yes. The entire storage/time/logging/persistence stack runs on the host against
a real temporary filesystem, including fault injection (full card, write
failure, mount failure).

**How do I simulate a full SD card?**
`nexus_storage_host_inject_full(true)` in a host test.

## Hardware

**Can it do CAN-FD?**
No. The STM32F746 has bxCAN 2.0B only. CAN-FD would require an external
controller, which the architecture leaves room for.

**Why is the Arduino header 3.3 V?**
The board's Arduino Uno V3 headers expose 3.3 V logic. Do not drive them with
5 V signals.

**Is the LCD/touch supported yet?**
Not yet. Those drivers are not integrated; the self-test reports them as
not-testable.

## Development

**Where do I edit code?**
`firmware/src` and `firmware/include`. The `firmware/wattlab_nexus` sketch
folder is generated — do not edit it.

**Why is there a generated sketch folder?**
Arduino only compiles sources inside the sketch directory. `sync-sketch.ps1`
mirrors the canonical sources there before building.
