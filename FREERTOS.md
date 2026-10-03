# WattLab Nexus - FreeRTOS Configuration

## Task Architecture

| Task | Priority | Stack | Description |
|------|----------|-------|-------------|
| Health | 5 | 2 KB | System monitoring, watchdog |
| Capture | 4 | 4 KB | Data acquisition |
| UI | 3 | 8 KB | Display rendering, touch |
| Protocol | 3 | 4 KB | Protocol engine processing |
| System | 2 | 4 KB | Initialization, coordination |
| Test | 2 | 4 KB | Automated test execution |
| Storage | 1 | 4 KB | File system operations |
| Logger | 1 | 2 KB | Log management |

## Synchronization

### Queues
- **UI Event Queue**: Touch events, button presses
- **Capture Data Queue**: Samples from capture engine
- **Protocol Event Queue**: Decoded protocol events
- **Log Queue**: Buffered records drained by the storage task

### Semaphores
- **SPI Bus**: SPI bus arbitration
- **I2C Bus**: I2C bus arbitration
- **SD Card**: Storage access

### Mutexes
- **Display**: LCD frame buffer access
- **Configuration**: Config read/write
- **Device Database**: Database access

## Storage I/O Rule

Slow SD writes must **never** run in UI, capture, protocol or ISR context.
The logger and capture paths enqueue records; the storage task (lowest
priority) drains them. This keeps high-priority tasks responsive while an SD
card stalls. See `STORAGE.md` for the queue depth and drop policy.

## Timing

| Operation | Period |
|-----------|--------|
| UI Refresh | 16 ms (60 FPS) |
| Capture Sampling | 1 ms |
| Protocol Processing | 5 ms |
| Health Check | 500 ms |
| Log Flush | 1000 ms |

## Memory Allocation

- **Task Stacks**: Statically allocated in internal SRAM
- **DMA Buffers**: Statically allocated in internal SRAM
- **Waveform Buffers**: Dynamically allocated in SDRAM
- **UI Assets**: Loaded from QSPI flash on demand

## Watchdog

- Independent watchdog (IWDG) with 1-second timeout
- Health task feeds watchdog every 500 ms
- If health task fails, system resets

## Power Management

- Sleep mode when idle
- Wake on touch, button, or communication event
- Peripheral clocks disabled when not in use
