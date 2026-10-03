# WattLab Nexus - Protocol Reference

## Supported Protocols

### UART
- Configurable baud rate (up to 115200+)
- 8N1, 8E1, 8O1, 7N1, etc.
- ASCII and HEX display modes
- Automatic baud detection
- TX/RX logging with timestamps
- Packet framing

### I2C
- Standard (100 kHz) and Fast (400 kHz) modes
- 7-bit and 10-bit addressing
- Bus scan and device identification
- Transaction capture (START, ADDR, DATA, STOP)
- ACK/NACK detection
- Clock stretching support

### SPI
- Configurable CPOL/CPHA (Mode 0-3)
- 8-bit and 16-bit word sizes
- MSB/LSB first
- Configurable clock rate
- CS polarity control
- Transaction capture

### CAN
- CAN 2.0B (not CAN-FD)
- Standard (11-bit) and Extended (29-bit) IDs
- Configurable bitrate (up to 1 Mbps)
- Filtering and masking
- TX/RX with timestamps
- Error detection

### GPIO
- Digital input/output
- Frequency measurement
- Duty cycle measurement
- Pulse width measurement
- Edge detection
- Transition counting

### 1-Wire
- Standard and overdrive modes
- ROM commands
- Device search
- CRC verification

## Protocol Event Timeline

All protocol events are timestamped and can be correlated on a single timeline:

```
00.000s  POWER ON
00.014s  GPIO D5 ↑
00.019s  UART TX 55
00.020s  UART RX AA
00.025s  I2C START
00.026s  I2C 0x68 ACK
00.030s  GPIO D5 ↓
```

## Protocol Analyzers

Each protocol has a dedicated analyzer that:
1. Receives raw events from the capture engine
2. Decodes protocol-specific frames
3. Produces human-readable output
4. Supports filtering and search

## Protocol Generators

Each protocol has a dedicated generator that:
1. Produces protocol-specific waveforms/patterns
2. Supports configurable parameters
3. Can be used for testing and simulation
4. Integrates with the test engine
