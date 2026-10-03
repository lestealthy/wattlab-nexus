# Security Policy

## Scope

WattLab Nexus is an embedded engineering workstation. This policy covers the
firmware, host tools and scripts in this repository.

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.2.x-beta | Yes (development) |
| 0.1.x | No |

## Reporting a vulnerability

Please do **not** open a public issue for security problems. Report privately to
the repository maintainer (open a private security advisory on GitHub, or
contact the maintainer directly). Include:

- affected version,
- a description of the issue and its impact,
- reproduction steps,
- any relevant logs or serial output.

## Security-relevant design notes

### Input validation

- All configuration and test files are treated as untrusted. JSON inputs are
  parsed into fixed-size structures with explicit bounds.
- Storage paths are validated (`nexus_storage_path_is_safe`) to prevent path
  traversal; filenames are sanitized to strip separators and reserved
  characters. It must not be possible to escape `/NEXUS`.
- The internal message protocol (`nexus_message`) rejects short buffers, bad
  magic, oversized payload lengths, truncated frames and CRC mismatches before
  dispatching.

### Bounds and memory

- No unbounded dynamic allocation. Buffers are fixed-size.
- String operations use bounded lengths; formatting helpers reject undersized
  buffers.
- CRC/checksum validation protects configuration and calibration blobs.

### Fault injection boundaries

- **Safe software faults** (UART byte drop/corrupt/inject, I2C NACK modelling,
  timeouts) are architecturally supported at the protocol layer.
- **Hardware fault injection** (supply glitching, bus contention, shorts) is
  out of scope and requires external circuitry. The firmware must never drive
  pins in a way that could damage the board or the device under test.

### Power control

- DUT power is a separate, protected subsystem. Firmware never connects
  arbitrary external DUT power to MCU pins. Over/under-voltage and over-current
  are modelled explicitly.

### Crash handling

- The fault handler only writes a compact record to backup SRAM. It never
  touches the filesystem. Persistence to SD happens at the next boot in normal
  context. Recovery is deterministic (reboot), never unsafe in-place resumption.

### Cryptographic material

- The firmware does not embed secrets. Do not commit tokens, keys or device
  credentials to this repository. The build environment's GitHub credentials
  are managed outside the repo.
