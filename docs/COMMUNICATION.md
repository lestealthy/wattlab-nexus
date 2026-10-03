# WattLab Nexus - Internal Communication Protocol

## Decision

Nexus uses **two complementary encodings**, chosen per boundary rather than one
format everywhere:

| Boundary | Format | Rationale |
|----------|--------|-----------|
| Internal task/queue bus | Framed binary (`nexus_message.h`) | Deterministic, allocation-free, cheap to parse on Cortex-M7 |
| File / user boundary (configs, tests, device DB) | JSON | Human-editable and diffable |
| Future PC companion payload | JSON or CBOR inside the binary frame | Transport stays constant; payload can evolve |

Rationale: JSON is excellent where humans edit files, but pulling a JSON
parser into every real-time task wastes flash and creates allocation risk on a
device with 320 KB SRAM. A fixed binary frame with a CRC gives the internal bus
predictable cost and easy corruption detection.

## Frame Format

Little-endian:

```
+--------+-------+--------+-------------+----------+--------+
| magic  | ver   | type   | payload_len | payload  | crc16  |
| 2 byte | 1 B   | 1 B    | 2 bytes     | N bytes  | 2 B    |
| 0x584D | 0x01  | see    | 0..256      |          | CCITT  |
+--------+-------+--------+-------------+----------+--------+
```

- `magic` = `0x584D` ("MX")
- `crc16` = CRC-16/CCITT-FALSE over header + payload
- `payload_len` is capped at `NEXUS_MSG_MAX_PAYLOAD` (256)

## Message Types

| Value | Type | Purpose |
|-------|------|---------|
| 0 | NONE | Reserved |
| 1 | EVENT | Protocol/capture event for the unified timeline |
| 2 | LOG | Structured log record |
| 3 | STATUS | System status update |
| 4 | COMMAND | UI/service command |
| 5 | RESPONSE | Command response |
| 6 | TEST_STEP | Test-engine step result |
| 7 | CAPTURE | Capture data block |
| 8 | HEARTBEAT | Liveness |

## Validation & Safety

Decoding rejects, in order:

1. Too-short buffers
2. Wrong magic
3. Payload length above the cap (before reading it)
4. Short frames for the declared length
5. CRC mismatch

These are covered by `tests/unit/test_message.cpp`.

## Event Timeline Correlation

All protocol/capture events carry a timestamp and share one timeline so that
power, GPIO, UART, I2C, SPI, CAN and ADC activity can be read in cause/effect
order. `nexus_event_t` (in `nexus_protocol.h`) is the in-memory event shape;
`NEXUS_MSG_EVENT` carries the serialized form across tasks and to the PC.
